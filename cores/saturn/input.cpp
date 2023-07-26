
#include "libretro.h"
#include "libretro_settings.h"
#include "mednafen/mednafen-types.h"
#include "mednafen/ss/ss.h"
#include "mednafen/ss/smpc.h"
#include "mednafen/state.h"
#include <math.h>
#include <stdio.h>

//------------------------------------------------------------------------------
// Locals
//------------------------------------------------------------------------------

#define MAX_CONTROLLERS		12 /* 2x 6 player adaptors */

static retro_environment_t environ_cb; /* cached during input_init_env */

static unsigned players				= 2;

static int astick_deadzone			= 0;
static int trigger_deadzone			= 0;
static const int TRIGGER_MAX			= 0xFFFF;
static float mouse_sensitivity			= 1.0f;

static unsigned geometry_width			= 0;
static unsigned geometry_height			= 0;

static int pointer_pressed			= 0;
static const int POINTER_PRESSED_CYCLES		= 4;
static int pointer_cycles_after_released	= 0;
static int pointer_pressed_last_x 		= 0;
static int pointer_pressed_last_y		= 0;

typedef union
{
	uint8_t u8[ 32 ];
	uint16_t gun_pos[ 2 ];
	uint16_t buttons;
} INPUT_DATA;

// Controller state buffer (per player)
static INPUT_DATA input_data[ MAX_CONTROLLERS ] = {0};

// Controller type (per player)
static uint32_t input_type[ MAX_CONTROLLERS ]	= {0};


#define INPUT_MODE_3D_PAD_ANALOG		( 1 << 0 ) // Set means analog mode.
#define INPUT_MODE_3D_PAD_PREVIOUS_MASK		( 1 << 1 ) // Edge trigger helper.
#define INPUT_MODE_MISSION_THROTTLE_LATCH	( 1 << 2 ) // Latch throttle enabled?
#define INPUT_MODE_MISSION_THROTTLE_PREV	( 1 << 3 ) // Edge trigger helper.

#define INPUT_MODE_DEFAULT				0
#define INPUT_MODE_DEFAULT_3D_PAD		INPUT_MODE_3D_PAD_ANALOG

// Mode switch for 3D Control Pad (per player)
static uint16_t input_mode[ MAX_CONTROLLERS ] 		= {0};
static int16_t input_throttle_latch[ MAX_CONTROLLERS ]	= {0};

//------------------------------------------------------------------------------
// Supported Devices
//------------------------------------------------------------------------------

#define RETRO_DEVICE_SS_PAD			RETRO_DEVICE_SUBCLASS( RETRO_DEVICE_JOYPAD, 0 )
#define RETRO_DEVICE_SS_3D_PAD		RETRO_DEVICE_SUBCLASS( RETRO_DEVICE_ANALOG, 0 )
#define RETRO_DEVICE_SS_WHEEL		RETRO_DEVICE_SUBCLASS( RETRO_DEVICE_ANALOG, 1 )
#define RETRO_DEVICE_SS_MISSION		RETRO_DEVICE_SUBCLASS( RETRO_DEVICE_ANALOG, 2 )
#define RETRO_DEVICE_SS_MISSION2	RETRO_DEVICE_SUBCLASS( RETRO_DEVICE_ANALOG, 3 )
#define RETRO_DEVICE_SS_MOUSE		RETRO_DEVICE_SUBCLASS( RETRO_DEVICE_MOUSE,  0 )
#define RETRO_DEVICE_SS_TWINSTICK	RETRO_DEVICE_SUBCLASS( RETRO_DEVICE_ANALOG, 4 )
#define RETRO_DEVICE_SS_GUN_JP		RETRO_DEVICE_SUBCLASS( RETRO_DEVICE_LIGHTGUN, 0 )
#define RETRO_DEVICE_SS_GUN_US		RETRO_DEVICE_SUBCLASS( RETRO_DEVICE_LIGHTGUN, 1 )

enum { INPUT_DEVICE_TYPES_COUNT = 1 /*none*/ + 9 }; /* <-- update me! */

static const struct retro_controller_description input_device_types[ INPUT_DEVICE_TYPES_COUNT ] =
{
	{ "Control Pad", RETRO_DEVICE_JOYPAD },
	{ "3D Control Pad", RETRO_DEVICE_SS_3D_PAD },
	{ "Arcade Racer", RETRO_DEVICE_SS_WHEEL },
	{ "Mission Stick", RETRO_DEVICE_SS_MISSION },
	{ "Mouse", RETRO_DEVICE_SS_MOUSE },
	{ "Stunner", RETRO_DEVICE_SS_GUN_US },
	{ "Twin-Stick", RETRO_DEVICE_SS_TWINSTICK },
	{ "Virtua Gun", RETRO_DEVICE_SS_GUN_JP },
	{ "Dual Mission Sticks", RETRO_DEVICE_SS_MISSION2 }, /*"Panzer Dragoon Zwei" only!*/
	{ NULL, 0 },
};

static const struct retro_controller_info ports_no_6player[ 1 + 1 + 1 ] =
{
	/* port one */
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	
	/* port two */
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },

	{ 0 },
};

static const struct retro_controller_info ports_left_6player[ 6 + 1 + 1 ] =
{
	/* port one */
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },

	/* port two: 6player adaptor */
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	
	{ 0 },
};

static const struct retro_controller_info ports_right_6player[ 1 + 6 + 1 ] =
{
	/* port one */
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	
	/* port two: 6player adaptor */
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },

	{ 0 },
};

static const struct retro_controller_info ports_two_6player[ 6 + 6 + 1 ] =
{
	/* port one: 6player adaptor */
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	
	/* port two: 6player adaptor */
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },
	{ input_device_types, INPUT_DEVICE_TYPES_COUNT },

	{ 0 },
};

/* Lookup table to select ports info for all combinations
   of 6player adaptor connection. */
static const struct retro_controller_info* ports_lut[ 4 ] =
{
	ports_no_6player,
	ports_left_6player,
	ports_right_6player,
	ports_two_6player
};


//------------------------------------------------------------------------------
// Mapping Helpers
//------------------------------------------------------------------------------

/* Control Pad (default) */
enum { INPUT_MAP_PAD_SIZE = 12 };
static const unsigned input_map_pad[ INPUT_MAP_PAD_SIZE ] =
{
	// libretro input				 at position	|| maps to Saturn		on bit
	//-----------------------------------------------------------------------------
	RETRO_DEVICE_ID_JOYPAD_L,		// L1			-> Z					0
	RETRO_DEVICE_ID_JOYPAD_X,		// X(top)		-> Y					1
	RETRO_DEVICE_ID_JOYPAD_Y,		// Y(left)		-> X					2
	RETRO_DEVICE_ID_JOYPAD_R2,		// R2			-> R					3
	RETRO_DEVICE_ID_JOYPAD_UP,		// Pad-Up		-> Pad-Up				4
	RETRO_DEVICE_ID_JOYPAD_DOWN,		// Pad-Down		-> Pad-Down				5
	RETRO_DEVICE_ID_JOYPAD_LEFT,		// Pad-Left		-> Pad-Left				6
	RETRO_DEVICE_ID_JOYPAD_RIGHT,		// Pad-Right		-> Pad-Right				7
	RETRO_DEVICE_ID_JOYPAD_A,		// A(right)		-> B					8
	RETRO_DEVICE_ID_JOYPAD_R,		// R1			-> C					9
	RETRO_DEVICE_ID_JOYPAD_B,		// B(down)		-> A					10
	RETRO_DEVICE_ID_JOYPAD_START,		// Start		-> Start				11
};

static const unsigned input_map_pad_left_shoulder =
	RETRO_DEVICE_ID_JOYPAD_L2;		// L2			-> L					15

/* 3D Control Pad */
enum { INPUT_MAP_3D_PAD_SIZE = 11 };
static const unsigned input_map_3d_pad[ INPUT_MAP_3D_PAD_SIZE ] =
{
	// libretro input				 at position	|| maps to Saturn		on bit
	//-----------------------------------------------------------------------------
	RETRO_DEVICE_ID_JOYPAD_UP,		// Pad-Up		-> Pad-Up				0
	RETRO_DEVICE_ID_JOYPAD_DOWN,		// Pad-Down		-> Pad-Down				1
	RETRO_DEVICE_ID_JOYPAD_LEFT,		// Pad-Left		-> Pad-Left				2
	RETRO_DEVICE_ID_JOYPAD_RIGHT,		// Pad-Right		-> Pad-Right				3
	RETRO_DEVICE_ID_JOYPAD_A,		// A(right)		-> B					4
	RETRO_DEVICE_ID_JOYPAD_R,		// R1			-> C					5
	RETRO_DEVICE_ID_JOYPAD_B,		// B(down)		-> A					6
	RETRO_DEVICE_ID_JOYPAD_START,		// Start		-> Start				7
	RETRO_DEVICE_ID_JOYPAD_L,		// L1			-> Z					8
	RETRO_DEVICE_ID_JOYPAD_X,		// X(top)		-> Y					9
	RETRO_DEVICE_ID_JOYPAD_Y,		// Y(left)		-> X					10
};

static const unsigned input_map_3d_pad_mode_switch = RETRO_DEVICE_ID_JOYPAD_SELECT;

/* Arcade Racer (wheel) */
enum { INPUT_MAP_WHEEL_BITSHIFT = 4 };
enum { INPUT_MAP_WHEEL_SIZE = 7 };
static const unsigned input_map_wheel[ INPUT_MAP_WHEEL_SIZE ] =
{
	// libretro input				 at position	|| maps to Saturn		on bit
	//-----------------------------------------------------------------------------
	RETRO_DEVICE_ID_JOYPAD_A,		// A(right)		-> B					4
	RETRO_DEVICE_ID_JOYPAD_R,		// R1			-> C					5
	RETRO_DEVICE_ID_JOYPAD_B,		// B(down)		-> A					6
	RETRO_DEVICE_ID_JOYPAD_START,		// Start		-> Start				7
	RETRO_DEVICE_ID_JOYPAD_L,		// L1			-> Z					8
	RETRO_DEVICE_ID_JOYPAD_X,		// X(top)		-> Y					9
	RETRO_DEVICE_ID_JOYPAD_Y,		// Y(left)		-> X					10
};

static const unsigned input_map_wheel_shift_left  = RETRO_DEVICE_ID_JOYPAD_L2;
static const unsigned input_map_wheel_shift_right = RETRO_DEVICE_ID_JOYPAD_R2;

/* Mission Stick */
enum { INPUT_MAP_MISSION_SIZE = 8 };
static const unsigned input_map_mission[ INPUT_MAP_MISSION_SIZE ] =
{
	// libretro input				 at position	|| maps to Saturn		on bit
	//-----------------------------------------------------------------------------
	RETRO_DEVICE_ID_JOYPAD_A,		// A(right)		-> B					0
	RETRO_DEVICE_ID_JOYPAD_R,		// R1			-> C					1
	RETRO_DEVICE_ID_JOYPAD_B,		// B(down)		-> A					2
	RETRO_DEVICE_ID_JOYPAD_START,		// Start		-> Start				3
	RETRO_DEVICE_ID_JOYPAD_L,		// L1			-> Z					4
	RETRO_DEVICE_ID_JOYPAD_X,		// X(top)		-> Y					5
	RETRO_DEVICE_ID_JOYPAD_Y,		// Y(left)		-> X					6
	RETRO_DEVICE_ID_JOYPAD_R2,		// R2			-> R					7
};

static const unsigned input_map_mission_left_shoulder =
	RETRO_DEVICE_ID_JOYPAD_L2;		// L2			-> L					15

static const unsigned input_map_mission_throttle_latch = RETRO_DEVICE_ID_JOYPAD_R3;

/* Twin-Stick */
static const unsigned input_map_twinstick_left_trigger  = RETRO_DEVICE_ID_JOYPAD_L2;
static const unsigned input_map_twinstick_left_button   = RETRO_DEVICE_ID_JOYPAD_L;
static const unsigned input_map_twinstick_right_trigger = RETRO_DEVICE_ID_JOYPAD_R2;
static const unsigned input_map_twinstick_right_button  = RETRO_DEVICE_ID_JOYPAD_R;

//------------------------------------------------------------------------------
// Local Functions
//------------------------------------------------------------------------------

static void get_analog_axis( retro_input_state_t input_state_cb,
		int player_index,
		int stick,
		int axis,
		int* p_analog )
{
	int analog = input_state_cb( player_index, RETRO_DEVICE_ANALOG, stick, axis );

	// Analog stick deadzone
	if ( astick_deadzone > 0 )
	{
		static const int ASTICK_MAX = 0x8000;
		const float scale           = ((float)ASTICK_MAX/(float)(ASTICK_MAX - astick_deadzone));

		if ( analog < -astick_deadzone )
		{
			// Re-scale analog stick range
			float scaled = (-analog - astick_deadzone)*scale;
			analog       = (int)round(-scaled);
			if (analog < -32767)
				analog = -32767;
		}
		else if ( analog > astick_deadzone )
		{
			// Re-scale analog stick range
			float scaled = (analog - astick_deadzone)*scale;
			analog       = (int)round(scaled);
			if (analog > +32767)
				analog = +32767;
		}
		else
			analog = 0;
	}

	// output
	*p_analog = analog;
}

static void get_analog_stick( retro_input_state_t input_state_cb,
		int player_index,
		int stick,
		int* p_analog_x,
		int* p_analog_y )
{
	int analog_x = input_state_cb( player_index, RETRO_DEVICE_ANALOG, stick, RETRO_DEVICE_ID_ANALOG_X );
	int analog_y = input_state_cb( player_index, RETRO_DEVICE_ANALOG, stick, RETRO_DEVICE_ID_ANALOG_Y );

	// Analog stick deadzone (borrowed code from parallel-n64 core)
	if ( astick_deadzone > 0 )
	{
		static const int ASTICK_MAX = 0x8000;

		// Convert cartesian coordinate analog stick to polar coordinates
		double radius = sqrt(analog_x * analog_x + analog_y * analog_y);
		double angle  = atan2(analog_y, analog_x);

		if (radius > astick_deadzone)
		{
			// Re-scale analog stick range to negate deadzone (makes slow movements possible)
			radius = (radius - astick_deadzone)*((float)ASTICK_MAX/(ASTICK_MAX - astick_deadzone));

			// Convert back to cartesian coordinates
			analog_x = (int)round(radius * cos(angle));
			analog_y = (int)round(radius * sin(angle));

			// Clamp to correct range
			if (analog_x > +32767) analog_x = +32767;
			if (analog_x < -32767) analog_x = -32767;
			if (analog_y > +32767) analog_y = +32767;
			if (analog_y < -32767) analog_y = -32767;
		}
		else
		{
			analog_x = 0;
			analog_y = 0;
		}
	}

	// output
	*p_analog_x = analog_x;
	*p_analog_y = analog_y;
}

static uint32_t apply_trigger_deadzone( uint32_t input )
{
	// Scale by two and apply outer deadzone (about 1%)
	input = ( input * 66191 ) / 32768;
	
	// Inner deadzone
	if ( trigger_deadzone > 0 )
	{
		const float scale = ((float)TRIGGER_MAX/(float)(TRIGGER_MAX - trigger_deadzone));

		if ( input > trigger_deadzone )
		{
			// Re-scale analog range
			float scaled = (input - trigger_deadzone)*scale;
			input        = (int)round(scaled);
		}
		else
			input        = 0;
	}
	
	// Clamp
	if (input > TRIGGER_MAX)
		input = TRIGGER_MAX;

	return input;
}

static uint16_t get_analog_trigger( retro_input_state_t input_state_cb,
		int player_index,
		int id )
{
	// NOTE: Analog triggers were added Nov 2017. Not all front-ends support this
	// feature (or pre-date it) so we need to handle this in a graceful way.

	// First, try and get an analog value using the new libretro API constant
	uint16_t trigger = input_state_cb( player_index,
			RETRO_DEVICE_ANALOG,
			RETRO_DEVICE_INDEX_ANALOG_BUTTON,
			id );

	if ( trigger == 0 )
	{
		// If we got exactly zero, we're either not pressing the button, or the front-end
		// is not reporting analog values. We need to do a second check using the classic
		// digital API method, to at least get some response - better than nothing.

		// NOTE: If we're really just not holding the trigger, we're still going to get zero.
		if (input_state_cb( player_index,
					RETRO_DEVICE_JOYPAD,
					0,
					id ))
			return  0xFFFF;
		return 0;
	}

	// We got something, which means the front-end can handle analog buttons.
	// So we apply a deadzone to the input and use it.
	// Mednafen wants 0 - 65535 so we scale up from 0 - 32767
	return apply_trigger_deadzone( (unsigned)trigger );
}


//------------------------------------------------------------------------------
// Global Functions
//------------------------------------------------------------------------------

void input_init_env( retro_environment_t _environ_cb )
{
	// Cache this
	environ_cb = _environ_cb;

#define RETRO_DESCRIPTOR_BLOCK( _user )																				\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },								\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },								\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },								\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },								\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A Button" },								\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B Button" },								\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "C Button" },								\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X Button" },								\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y Button" },								\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Z Button" },								\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L Button" },								\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R Button" },								\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start Button" },							\
		{ _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Mode Switch" },							\
		{ _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },		\
		{ _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },		\
		{ _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },	\
		{ _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" },	\
		{ _user, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER, "Gun Trigger" },						\
		{ _user, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_START, "Gun Start" },							\
		{ _user, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD, "Gun Reload" }

	struct retro_input_descriptor desc[] =
	{
		RETRO_DESCRIPTOR_BLOCK( 0 ),
		RETRO_DESCRIPTOR_BLOCK( 1 ),
		RETRO_DESCRIPTOR_BLOCK( 2 ),
		RETRO_DESCRIPTOR_BLOCK( 3 ),
		RETRO_DESCRIPTOR_BLOCK( 4 ),
		RETRO_DESCRIPTOR_BLOCK( 5 ),
		RETRO_DESCRIPTOR_BLOCK( 6 ),
		RETRO_DESCRIPTOR_BLOCK( 7 ),
		RETRO_DESCRIPTOR_BLOCK( 8 ),
		RETRO_DESCRIPTOR_BLOCK( 9 ),
		RETRO_DESCRIPTOR_BLOCK( 10 ),
		RETRO_DESCRIPTOR_BLOCK( 11 ),

		{ 0 },
	};

#undef RETRO_DESCRIPTOR_BLOCK

	/* Send to front-end */
	environ_cb( RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc );
}

void input_set_env( retro_environment_t environ_cb )
{
	/* Pick ports selection */
	const struct retro_controller_info* ports = ports_lut[ setting_multitap_port1 + setting_multitap_port2 * 2 ];
	
	/* Send to front-end */
	environ_cb( RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports );
}

void input_init(void)
{
	// Initialise to default pad type and bind input buffers to SMPC emulation.
	for ( unsigned i = 0; i < MAX_CONTROLLERS; ++i )
	{
		input_type[ i ] = RETRO_DEVICE_JOYPAD;
		input_mode[ i ] = INPUT_MODE_DEFAULT;
		input_throttle_latch[ i ] = 0;

		SMPC_SetInput( i, "gamepad", (uint8*)&input_data[ i ] );
	}
}

void input_set_geometry( unsigned width, unsigned height )
{
	log_cb( RETRO_LOG_INFO, "input_set_geometry: %dx%d\n", width, height );

	geometry_width = width;
	geometry_height = height;
}

void input_set_deadzone_stick( int percent )
{
	if ( percent >= 0 && percent <= 100 )
		astick_deadzone = (int)( percent * 0.01f * 0x8000);
}

void input_set_deadzone_trigger( int percent )
{
	if ( percent >= 0 && percent <= 100 )
		trigger_deadzone = (int)( percent * 0.01f * TRIGGER_MAX);
}

void input_set_mouse_sensitivity( int percent )
{
	if ( percent > 0 && percent <= 200 )
		mouse_sensitivity = (float)percent / 100.0f;
}

void input_update_with_bitmasks( retro_input_state_t input_state_cb )
{
	// For each player (logical controller)
	for ( unsigned iplayer = 0; iplayer < players; ++iplayer )
	{
		INPUT_DATA* p_input = &(input_data[ iplayer ]);
		int16_t ret         = input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK );

		// reset input
		p_input->buttons    = 0;

		// What kind of controller is connected?
		switch ( input_type[ iplayer ] )
		{

		case RETRO_DEVICE_JOYPAD:
		case RETRO_DEVICE_SS_PAD:
	
			//
			// -- standard control pad buttons + d-pad

			// input_map_pad is configured to quickly map libretro buttons to the correct bits for the Saturn.
			for ( int i = 0; i < INPUT_MAP_PAD_SIZE; ++i )
			{
				if (ret & (1 << input_map_pad[ i ]))
					p_input->buttons |= ( 1 << i );
			}

			// .. the left trigger on the Saturn is a special case since there's a gap in the bits.
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_L2))
				p_input->buttons |= ( 1 << 15 );
 
			if (!opposite_directions)
			{
				if ((p_input->buttons & 0x30) == 0x30)
					p_input->buttons &= ~0x30;
				if ((p_input->buttons & 0xC0) == 0xC0)
					p_input->buttons &= ~0xC0;
			}
			break;

		case RETRO_DEVICE_SS_TWINSTICK:

			{
				int analog_lx, analog_ly;
				int analog_rx, analog_ry;
				get_analog_stick( input_state_cb, iplayer, RETRO_DEVICE_INDEX_ANALOG_LEFT, &analog_lx, &analog_ly );
				get_analog_stick( input_state_cb, iplayer, RETRO_DEVICE_INDEX_ANALOG_RIGHT, &analog_rx, &analog_ry );

				const int thresh = 16000;

				// left-stick
				if ( analog_ly <= -thresh )
					p_input->buttons |= ( 1 << 4 ); // Up
				if ( analog_lx >= thresh )
					p_input->buttons |= ( 1 << 7 ); // Right
				if ( analog_ly >= thresh )
					p_input->buttons |= ( 1 << 5 ); // Down
				if ( analog_lx <= -thresh )
					p_input->buttons |= ( 1 << 6 ); // Left

				// right-stick
				if ( analog_ry <= -thresh )
					p_input->buttons |= ( 1 << 1 ); // Up <-(Y)
				if ( analog_rx >= thresh )
					p_input->buttons |= ( 1 << 0 ); // Right <-(Z)
				if ( analog_ry >= thresh )
					p_input->buttons |= ( 1 << 8 ); // Down <-(B)
				if ( analog_rx <= -thresh )
					p_input->buttons |= ( 1 << 2 ); // Left <-(X)

				// left trigger
				if (ret & (1 << input_map_twinstick_left_trigger))
					p_input->buttons |= (1 << 15);

				// left button
				if (ret & (1 << input_map_twinstick_left_button))
					p_input->buttons |= ( 1 << 3 );

				// right trigger
				if (ret & (1 << input_map_twinstick_right_trigger))
					p_input->buttons |= ( 1 << 10 );

				// right button
				if (ret & (1 << input_map_twinstick_right_button))
					p_input->buttons |= ( 1 << 9 );

				// start
				if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_START))
					p_input->buttons |= ( 1 << 11 );
			}
			break;

		case RETRO_DEVICE_SS_3D_PAD:

			{
				//
				// -- 3d control pad buttons

				// input_map_3d_pad is configured to quickly map libretro buttons to the correct bits for the Saturn.
				for ( int i = 0; i < INPUT_MAP_3D_PAD_SIZE; ++i )
					if (ret & (1 << input_map_3d_pad[ i ]))
						p_input->buttons |= ( 1 << i );

				//
				// -- analog stick

				int analog_x, analog_y;
				get_analog_stick( input_state_cb, iplayer, RETRO_DEVICE_INDEX_ANALOG_LEFT, &analog_x, &analog_y );

				// mednafen wants 0 - 32767 - 65535
				uint16_t thumb_x = static_cast< uint16_t >( analog_x + 32767 );
				uint16_t thumb_y = static_cast< uint16_t >( analog_y + 32767 );

				//
				// -- triggers

				// mednafen wants 0 - 65535
				uint16_t l_trigger = get_analog_trigger( input_state_cb, iplayer, RETRO_DEVICE_ID_JOYPAD_L2 );
				uint16_t r_trigger = get_analog_trigger( input_state_cb, iplayer, RETRO_DEVICE_ID_JOYPAD_R2 );


				//
				// -- mode switch

				{
					// Handle MODE button as a switch
					uint16_t prev = ( input_mode[iplayer] & INPUT_MODE_3D_PAD_PREVIOUS_MASK );
					uint16_t held = 0;

					if (ret & (1 << input_map_3d_pad_mode_switch))
						held = INPUT_MODE_3D_PAD_PREVIOUS_MASK;

					// Rising edge trigger
					if ( !prev && held )
					{
						char text[ 256 ];
						// Toggle 'state' bit: analog/digital mode
						input_mode[ iplayer ] ^= INPUT_MODE_3D_PAD_ANALOG;

						// Tell user
						if ( input_mode[iplayer] & INPUT_MODE_3D_PAD_ANALOG )
							sprintf( text, "Controller %u: Analog Mode", (iplayer+1) );
						else
							sprintf( text, "Controller %u: Digital Mode", (iplayer+1) );
						struct retro_message msg = { text, 180 };
						environ_cb( RETRO_ENVIRONMENT_SET_MESSAGE, &msg );
					}

					// Store held state in 'previous' bit.
					input_mode[ iplayer ] = ( input_mode[ iplayer ] & ~INPUT_MODE_3D_PAD_PREVIOUS_MASK ) | held;
				}

				//
				// -- format input data

				// Apply analog/digital mode switch bit.
				if ( input_mode[iplayer] & INPUT_MODE_3D_PAD_ANALOG )
					p_input->buttons |= 0x1000; // set bit 12

				p_input->u8[0x2] = ((thumb_x >> 0) & 0xff);
				p_input->u8[0x3] = ((thumb_x >> 8) & 0xff);
				p_input->u8[0x4] = ((thumb_y >> 0) & 0xff);
				p_input->u8[0x5] = ((thumb_y >> 8) & 0xff);
				p_input->u8[0x6] = ((r_trigger >> 0) & 0xff);
				p_input->u8[0x7] = ((r_trigger >> 8) & 0xff);
				p_input->u8[0x8] = ((l_trigger >> 0) & 0xff);
				p_input->u8[0x9] = ((l_trigger >> 8) & 0xff);
			}

			break;

		case RETRO_DEVICE_SS_WHEEL:

			{
				//
				// -- Wheel buttons

				// input_map_wheel is configured to quickly map libretro buttons to the correct bits for the Saturn.
				for ( int i = 0; i < INPUT_MAP_WHEEL_SIZE; ++i )
				{
					const uint16_t bit = ( 1 << ( i + INPUT_MAP_WHEEL_BITSHIFT ) );
					if (ret & (1 << input_map_wheel[ i ]))
						p_input->buttons |= bit;
				}

				// shift-paddles
				if (ret & (1 << input_map_wheel_shift_left))
					p_input->buttons |= ( 1 << 0 );
				if (ret & (1 << input_map_wheel_shift_right))
					p_input->buttons |= ( 1 << 1 );

				//
				// -- analog wheel

				int analog_x;
				get_analog_axis( input_state_cb, iplayer,
					RETRO_DEVICE_INDEX_ANALOG_LEFT,
					RETRO_DEVICE_ID_ANALOG_X, &analog_x );

				//
				// -- format input data

				// Convert analog values into direction values.
				uint16_t right   = analog_x > 0 ?  analog_x : 0;
				uint16_t left    = analog_x < 0 ? -analog_x : 0;

				p_input->u8[0x2] = ((left  >> 0) & 0xff);
				p_input->u8[0x3] = ((left  >> 8) & 0xff);
				p_input->u8[0x4] = ((right >> 0) & 0xff);
				p_input->u8[0x5] = ((right >> 8) & 0xff);
			}

			break;

		case RETRO_DEVICE_SS_MOUSE:

			{
				// mouse buttons
				p_input->u8[0x4] = 0;

				if ( input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT ) )
					p_input->u8[0x4] |= ( 1 << 0 ); // A

				if ( input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT ) )
					p_input->u8[0x4] |= ( 1 << 1 ); // B

				if ( input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE ) )
					p_input->u8[0x4] |= ( 1 << 2 ); // C

				if ( input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START ) ||
					 input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_BUTTON_4 ) ||
					 input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_BUTTON_5 ) ) {
					p_input->u8[0x4] |= ( 1 << 3 ); // Start
				}

				// mouse input
				int dx_raw = input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X );
				int dy_raw = input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y );

				int16_t *delta = (int16_t*)p_input;
				delta[ 0 ] = (int16_t)roundf( dx_raw * mouse_sensitivity );
				delta[ 1 ] = (int16_t)roundf( dy_raw * mouse_sensitivity );
			}

			break;

		case RETRO_DEVICE_SS_MISSION:

			{
				//
				// -- mission stick buttons

				// input_map_mission is configured to quickly map libretro buttons to the correct bits for the Saturn.
				for ( int i = 0; i < INPUT_MAP_MISSION_SIZE; ++i )
					if (ret & (1 << input_map_mission[ i ]))
						p_input->buttons |= ( 1 << i );

				// .. the left trigger is a special case, there's a gap in the bits.
				if (ret & (1 << input_map_mission_left_shoulder))
					p_input->buttons |= ( 1 << 11 );

				//
				// -- analog stick

				int analog_x, analog_y;
				get_analog_stick( input_state_cb, iplayer, RETRO_DEVICE_INDEX_ANALOG_LEFT, &analog_x, &analog_y );

				//
				// -- throttle

				int throttle_real;
				get_analog_axis( input_state_cb, iplayer,
					RETRO_DEVICE_INDEX_ANALOG_RIGHT,
					RETRO_DEVICE_ID_ANALOG_Y, &throttle_real );

				int16_t throttle;
				if ( input_mode[iplayer] & INPUT_MODE_MISSION_THROTTLE_LATCH ) // Use latched value
					throttle = input_throttle_latch[iplayer];
				else // Direct read
					throttle = throttle_real;


				//
				// -- throttle latch switch

				{
					// Handle MODE button as a switch
					uint16_t prev = ( input_mode[iplayer] & INPUT_MODE_MISSION_THROTTLE_PREV );
					uint16_t held = 0;

					if (ret & (1 << input_map_mission_throttle_latch))
						held  = INPUT_MODE_MISSION_THROTTLE_PREV;

					// Rising edge trigger?
					if ( !prev && held )
					{
						// Toggle 'state' bit: throttle latch enable/disable.
						input_mode[ iplayer ] ^= INPUT_MODE_MISSION_THROTTLE_LATCH;

						// Tell user
						if ( input_mode[iplayer] & INPUT_MODE_MISSION_THROTTLE_LATCH )
							input_throttle_latch[iplayer] = (int16_t)throttle_real;
					}

					// Store held state in 'previous' bit.
					input_mode[ iplayer ] = ( input_mode[ iplayer ] & ~INPUT_MODE_MISSION_THROTTLE_PREV ) | held;
				}


				//
				// -- format input data

				// Convert analog values into direction values.
				uint16_t right = analog_x > 0 ?  analog_x : 0;
				uint16_t left  = analog_x < 0 ? -analog_x : 0;
				uint16_t down  = analog_y > 0 ?  analog_y : 0;
				uint16_t up    = analog_y < 0 ? -analog_y : 0;
				uint16_t th_up = throttle > 0 ?  throttle : 0;
				uint16_t th_dn = throttle < 0 ? -throttle : 0;

				p_input->u8[0x2] = 0; // todo: auto-fire controls.
				p_input->u8[0x3] = ((left  >> 0) & 0xff);
				p_input->u8[0x4] = ((left  >> 8) & 0xff);
				p_input->u8[0x5] = ((right >> 0) & 0xff);
				p_input->u8[0x6] = ((right >> 8) & 0xff);
				p_input->u8[0x7] = ((up    >> 0) & 0xff);
				p_input->u8[0x8] = ((up    >> 8) & 0xff);
				p_input->u8[0x9] = ((down  >> 0) & 0xff);
				p_input->u8[0xa] = ((down  >> 8) & 0xff);
				p_input->u8[0xb] = ((th_up >> 0) & 0xff);
				p_input->u8[0xc] = ((th_up >> 8) & 0xff);
				p_input->u8[0xd] = ((th_dn >> 0) & 0xff);
				p_input->u8[0xe] = ((th_dn >> 8) & 0xff);
			}

			break;

		case RETRO_DEVICE_SS_MISSION2:

			{
				//
				// -- mission stick buttons

				// input_map_mission is configured to quickly map libretro buttons to the correct bits for the Saturn.
				for ( int i = 0; i < INPUT_MAP_MISSION_SIZE; ++i )
				{
					if (ret & (1 << input_map_mission[ i ]))
						p_input->buttons |= ( 1 << i );
				}
				// .. the left trigger is a special case, there's a gap in the bits.
				if (ret & (1 << input_map_mission_left_shoulder))
					p_input->buttons |= ( 1 << 11 );

				//
				// -- analog sticks

				int analog1_x, analog1_y;
				int analog2_x, analog2_y;

				// Default - patent shows first stick on right side, second added on left
				// see: https://segaretro.org/images/a/a1/Patent_EP0745928A2.pdf
				get_analog_stick( input_state_cb, iplayer, RETRO_DEVICE_INDEX_ANALOG_RIGHT, &analog1_x, &analog1_y );
				get_analog_stick( input_state_cb, iplayer, RETRO_DEVICE_INDEX_ANALOG_LEFT, &analog2_x, &analog2_y );

				//
				// -- format input data

				// Convert analog values into direction values.
				uint16_t right1 = analog1_x > 0 ?  analog1_x : 0;
				uint16_t left1  = analog1_x < 0 ? -analog1_x : 0;
				uint16_t down1  = analog1_y > 0 ?  analog1_y : 0;
				uint16_t up1    = analog1_y < 0 ? -analog1_y : 0;

				uint16_t right2 = analog2_x > 0 ?  analog2_x : 0;
				uint16_t left2  = analog2_x < 0 ? -analog2_x : 0;
				uint16_t down2  = analog2_y > 0 ?  analog2_y : 0;
				uint16_t up2    = analog2_y < 0 ? -analog2_y : 0;

				p_input->u8[ 0x2] = 0; // todo: auto-fire controls.

				p_input->u8[ 0x3] = ((left1  >> 0) & 0xff);
				p_input->u8[ 0x4] = ((left1  >> 8) & 0xff);
				p_input->u8[ 0x5] = ((right1 >> 0) & 0xff);
				p_input->u8[ 0x6] = ((right1 >> 8) & 0xff);
				p_input->u8[ 0x7] = ((up1    >> 0) & 0xff);
				p_input->u8[ 0x8] = ((up1    >> 8) & 0xff);
				p_input->u8[ 0x9] = ((down1  >> 0) & 0xff);
				p_input->u8[ 0xa] = ((down1  >> 8) & 0xff);
				p_input->u8[ 0xb] = 0; // todo: throttle1
				p_input->u8[ 0xc] = 0;
				p_input->u8[ 0xd] = 0;
				p_input->u8[ 0xe] = 0;

				p_input->u8[ 0xf] = ((left2  >> 0) & 0xff);
				p_input->u8[0x10] = ((left2  >> 8) & 0xff);
				p_input->u8[0x11] = ((right2 >> 0) & 0xff);
				p_input->u8[0x12] = ((right2 >> 8) & 0xff);
				p_input->u8[0x13] = ((up2    >> 0) & 0xff);
				p_input->u8[0x14] = ((up2    >> 8) & 0xff);
				p_input->u8[0x15] = ((down2  >> 0) & 0xff);
				p_input->u8[0x16] = ((down2  >> 8) & 0xff);
				p_input->u8[0x17] = 0; // todo: throttle2
				p_input->u8[0x18] = 0;
				p_input->u8[0x19] = 0;
				p_input->u8[0x1a] = 0;
			}

			break;

		case RETRO_DEVICE_SS_GUN_JP:
		case RETRO_DEVICE_SS_GUN_US:

			{
				if ( setting_gun_input == SETTING_GUN_INPUT_POINTER )
				{
					int gun_x_raw = input_state_cb( iplayer, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
					int gun_y_raw = input_state_cb( iplayer, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

					// .. scale into screen space:
					// NOTE: the scaling here is empirical-guesswork.
					// Tested at 352x240 (ntsc) and 352x256 (pal)

					const int scale_x  = 21472;
					const int scale_y  = geometry_height;
					const int offset_y = geometry_height - 240;

					int is_offscreen   = 0;

					int gun_x = ( ( gun_x_raw + 0x7fff ) * scale_x ) / (0x7fff << 1);
					int gun_y = ( ( gun_y_raw + 0x7fff ) * scale_y ) / (0x7fff << 1) + offset_y;

					// Handle offscreen by checking corrected x and y values
					if ( gun_x == 0 || gun_y == 0 )
					{
						is_offscreen = 1;
						gun_x        = -16384; // magic position to disable cross-hair drawing.
						gun_y        = -16384;
					}

					// Touch sensitivity: Keep the gun position held for a fixed number of cycles after touch is released
					// because a very light touch can result in a misfire
					if ( pointer_cycles_after_released > 0 && pointer_cycles_after_released < POINTER_PRESSED_CYCLES ) {
						pointer_cycles_after_released++;
						p_input->gun_pos[ 0 ] = pointer_pressed_last_x;
						p_input->gun_pos[ 1 ] = pointer_pressed_last_y;
						return;
					}

					// trigger
					if ( input_state_cb( iplayer, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED ) )
					{
						pointer_pressed = 1;
						pointer_cycles_after_released = 0;
						pointer_pressed_last_x = gun_x;
						pointer_pressed_last_y = gun_y;
					} else if ( pointer_pressed ) {
						pointer_cycles_after_released++;
						pointer_pressed = 0;
						p_input->gun_pos[ 0 ] = pointer_pressed_last_x;
						p_input->gun_pos[ 1 ] = pointer_pressed_last_y;
						p_input->u8[4] &= ~0x1;
						return;
					}

					// position
					p_input->gun_pos[ 0 ] = gun_x;
					p_input->gun_pos[ 1 ] = gun_y;

					// buttons
					p_input->u8[ 4 ] = 0;

					// use multi-touch to support different button inputs:
					// 3-finger touch: START button
					// 2-finger touch: Reload
					// offscreen touch: Reload
					int touch_count = input_state_cb( iplayer, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_COUNT );
					if ( touch_count == 3 )
						p_input->u8[ 4 ] |= 0x2;
					else if ( touch_count == 2 )
						p_input->u8[ 4 ] |= 0x4;
					else if ( touch_count == 1 && is_offscreen )
						p_input->u8[ 4 ] |= 0x4;
					else if ( touch_count == 1 )
						p_input->u8[ 4 ] |= 0x1;

				} else {   // Lightgun input is default
					uint8_t shot_type;
					int gun_x, gun_y;
					int forced_reload = input_state_cb( iplayer, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD );

					// off-screen?
					if ( input_state_cb( iplayer, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN ) ||
						forced_reload ||
						geometry_height == 0 )
					{
						shot_type = 0x4; // off-screen shot

						gun_x = -16384; // magic position to disable cross-hair drawing.
						gun_y = -16384;
					}
					else
					{
						shot_type = 0x1; // on-screen shot

						int gun_x_raw = input_state_cb( iplayer, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X );
						int gun_y_raw = input_state_cb( iplayer, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y );

						// .. scale into screen space:
						// NOTE: the scaling here is empirical-guesswork.
						// Tested at 352x240 (ntsc) and 352x256 (pal)

						const int scale_x = 21472;
						const int scale_y = geometry_height;
						const int offset_y = geometry_height - 240;

						gun_x = ( ( gun_x_raw + 0x7fff ) * scale_x ) / (0x7fff << 1);
						gun_y = ( ( gun_y_raw + 0x7fff ) * scale_y ) / (0x7fff << 1) + offset_y;
					}

					// position
					p_input->gun_pos[ 0 ] = gun_x;
					p_input->gun_pos[ 1 ] = gun_y;

					// buttons
					p_input->u8[ 4 ] = 0;

					// trigger
					if ( input_state_cb( iplayer, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER ) || forced_reload )
						p_input->u8[ 4 ] |= shot_type;

					// start
					if ( input_state_cb( iplayer, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_START ) )
						p_input->u8[ 4 ] |= 0x2;

				}

			}

			break;

		} // switch ( input_type[ iplayer ] )

	} // for each player
}

void input_update( retro_input_state_t input_state_cb )
{
	// For each player (logical controller)
	for ( unsigned iplayer = 0; iplayer < players; ++iplayer )
	{
		INPUT_DATA* p_input = &(input_data[ iplayer ]);

		// reset input
		p_input->buttons = 0;

		// What kind of controller is connected?
		switch ( input_type[ iplayer ] )
		{

		case RETRO_DEVICE_JOYPAD:
		case RETRO_DEVICE_SS_PAD:
	
			//
			// -- standard control pad buttons + d-pad

			// input_map_pad is configured to quickly map libretro buttons to the correct bits for the Saturn.
			for ( int i = 0; i < INPUT_MAP_PAD_SIZE; ++i )
				p_input->buttons |= input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_pad[ i ] ) ? ( 1 << i ) : 0;
			// .. the left trigger on the Saturn is a special case since there's a gap in the bits.
			p_input->buttons |=
				input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_pad_left_shoulder ) ? ( 1 << 15 ) : 0;
 
			if (!opposite_directions)
			{
				if ((p_input->buttons & 0x30) == 0x30)
					p_input->buttons &= ~0x30;
				if ((p_input->buttons & 0xC0) == 0xC0)
					p_input->buttons &= ~0xC0;
			}
			break;

		case RETRO_DEVICE_SS_TWINSTICK:

			{
				int analog_lx, analog_ly;
				get_analog_stick( input_state_cb, iplayer, RETRO_DEVICE_INDEX_ANALOG_LEFT, &analog_lx, &analog_ly );
				int analog_rx, analog_ry;
				get_analog_stick( input_state_cb, iplayer, RETRO_DEVICE_INDEX_ANALOG_RIGHT, &analog_rx, &analog_ry );

				const int thresh = 16000;

				// left-stick
				if ( analog_ly <= -thresh )
					p_input->buttons |= ( 1 << 4 ); // Up
				if ( analog_lx >= thresh )
					p_input->buttons |= ( 1 << 7 ); // Right
				if ( analog_ly >= thresh )
					p_input->buttons |= ( 1 << 5 ); // Down
				if ( analog_lx <= -thresh )
					p_input->buttons |= ( 1 << 6 ); // Left

				// right-stick
				if ( analog_ry <= -thresh )
					p_input->buttons |= ( 1 << 1 ); // Up <-(Y)
				if ( analog_rx >= thresh )
					p_input->buttons |= ( 1 << 0 ); // Right <-(Z)
				if ( analog_ry >= thresh )
					p_input->buttons |= ( 1 << 8 ); // Down <-(B)
				if ( analog_rx <= -thresh )
					p_input->buttons |= ( 1 << 2 ); // Left <-(X)

				// left trigger
				p_input->buttons |=
					input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_twinstick_left_trigger ) ? ( 1 << 15 ) : 0;

				// left button
				p_input->buttons |=
					input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_twinstick_left_button ) ? ( 1 << 3 ) : 0;

				// right trigger
				p_input->buttons |=
					input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_twinstick_right_trigger ) ? ( 1 << 10 ) : 0;

				// right button
				p_input->buttons |=
					input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_twinstick_right_button ) ? ( 1 << 9 ) : 0;

				// start
				p_input->buttons |=
					input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START ) ? ( 1 << 11 ) : 0;
			}
			break;

		case RETRO_DEVICE_SS_3D_PAD:

			{
				//
				// -- 3d control pad buttons

				// input_map_3d_pad is configured to quickly map libretro buttons to the correct bits for the Saturn.

				for ( int i = 0; i < INPUT_MAP_3D_PAD_SIZE; ++i )
					p_input->buttons |= input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_3d_pad[ i ] ) ? ( 1 << i ) : 0;

				//
				// -- analog stick

				int analog_x, analog_y;
				get_analog_stick( input_state_cb, iplayer, RETRO_DEVICE_INDEX_ANALOG_LEFT, &analog_x, &analog_y );

				// mednafen wants 0 - 32767 - 65535
				uint16_t thumb_x, thumb_y;
				thumb_x = static_cast< uint16_t >( analog_x + 32767 );
				thumb_y = static_cast< uint16_t >( analog_y + 32767 );

				//
				// -- triggers

				// mednafen wants 0 - 65535
				uint16_t l_trigger, r_trigger;
				l_trigger = get_analog_trigger( input_state_cb, iplayer, RETRO_DEVICE_ID_JOYPAD_L2 );
				r_trigger = get_analog_trigger( input_state_cb, iplayer, RETRO_DEVICE_ID_JOYPAD_R2 );


				//
				// -- mode switch

				{
					// Handle MODE button as a switch
					uint16_t prev = ( input_mode[iplayer] & INPUT_MODE_3D_PAD_PREVIOUS_MASK );
					uint16_t held = 0;

					if (input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_3d_pad_mode_switch ))
						held = INPUT_MODE_3D_PAD_PREVIOUS_MASK;

					// Rising edge trigger
					if ( !prev && held )
					{
						char text[ 256 ];
						// Toggle 'state' bit: analog/digital mode
						input_mode[ iplayer ] ^= INPUT_MODE_3D_PAD_ANALOG;

						// Tell user
						if ( input_mode[iplayer] & INPUT_MODE_3D_PAD_ANALOG )
							sprintf( text, "Controller %u: Analog Mode", (iplayer+1) );
						else
							sprintf( text, "Controller %u: Digital Mode", (iplayer+1) );
						struct retro_message msg = { text, 180 };
						environ_cb( RETRO_ENVIRONMENT_SET_MESSAGE, &msg );
					}

					// Store held state in 'previous' bit.
					input_mode[ iplayer ] = ( input_mode[ iplayer ] & ~INPUT_MODE_3D_PAD_PREVIOUS_MASK ) | held;
				}

				//
				// -- format input data

				// Apply analog/digital mode switch bit.
				if ( input_mode[iplayer] & INPUT_MODE_3D_PAD_ANALOG ) {
					p_input->buttons |= 0x1000; // set bit 12
				}

				p_input->u8[0x2] = ((thumb_x >> 0) & 0xff);
				p_input->u8[0x3] = ((thumb_x >> 8) & 0xff);
				p_input->u8[0x4] = ((thumb_y >> 0) & 0xff);
				p_input->u8[0x5] = ((thumb_y >> 8) & 0xff);
				p_input->u8[0x6] = ((r_trigger >> 0) & 0xff);
				p_input->u8[0x7] = ((r_trigger >> 8) & 0xff);
				p_input->u8[0x8] = ((l_trigger >> 0) & 0xff);
				p_input->u8[0x9] = ((l_trigger >> 8) & 0xff);
			}

			break;

		case RETRO_DEVICE_SS_WHEEL:

			{
				//
				// -- Wheel buttons

				// input_map_wheel is configured to quickly map libretro buttons to the correct bits for the Saturn.
				for ( int i = 0; i < INPUT_MAP_WHEEL_SIZE; ++i ) {
					const uint16_t bit = ( 1 << ( i + INPUT_MAP_WHEEL_BITSHIFT ) );
					p_input->buttons |= input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_wheel[ i ] ) ? bit : 0;
				}

				// shift-paddles
				p_input->buttons |= input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_wheel_shift_left ) ? ( 1 << 0 ) : 0;
				p_input->buttons |= input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_wheel_shift_right ) ? ( 1 << 1 ) : 0;

				//
				// -- analog wheel

				int analog_x;
				get_analog_axis( input_state_cb, iplayer,
					RETRO_DEVICE_INDEX_ANALOG_LEFT,
					RETRO_DEVICE_ID_ANALOG_X, &analog_x );

				//
				// -- format input data

				// Convert analog values into direction values.
				uint16_t right = analog_x > 0 ?  analog_x : 0;
				uint16_t left  = analog_x < 0 ? -analog_x : 0;

				p_input->u8[0x2] = ((left  >> 0) & 0xff);
				p_input->u8[0x3] = ((left  >> 8) & 0xff);
				p_input->u8[0x4] = ((right >> 0) & 0xff);
				p_input->u8[0x5] = ((right >> 8) & 0xff);
			}

			break;

		case RETRO_DEVICE_SS_MOUSE:

			{
				// mouse buttons
				p_input->u8[0x4] = 0;

				if ( input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT ) ) {
					p_input->u8[0x4] |= ( 1 << 0 ); // A
				}

				if ( input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT ) ) {
					p_input->u8[0x4] |= ( 1 << 1 ); // B
				}

				if ( input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE ) ) {
					p_input->u8[0x4] |= ( 1 << 2 ); // C
				}

				if ( input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START ) ||
					 input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_BUTTON_4 ) ||
					 input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_BUTTON_5 ) ) {
					p_input->u8[0x4] |= ( 1 << 3 ); // Start
				}

				// mouse input
				int dx_raw, dy_raw;
				dx_raw = input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X );
				dy_raw = input_state_cb( iplayer, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y );

				int16_t *delta;
				delta = (int16_t*)p_input;
				delta[ 0 ] = (int16_t)roundf( dx_raw * mouse_sensitivity );
				delta[ 1 ] = (int16_t)roundf( dy_raw * mouse_sensitivity );
			}

			break;

		case RETRO_DEVICE_SS_MISSION:

			{
				//
				// -- mission stick buttons

				// input_map_mission is configured to quickly map libretro buttons to the correct bits for the Saturn.
				for ( int i = 0; i < INPUT_MAP_MISSION_SIZE; ++i )
					p_input->buttons |= input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_mission[ i ] ) ? ( 1 << i ) : 0;
				// .. the left trigger is a special case, there's a gap in the bits.
				p_input->buttons |=
					input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_mission_left_shoulder ) ? ( 1 << 11 ) : 0;

				//
				// -- analog stick

				int analog_x, analog_y;
				get_analog_stick( input_state_cb, iplayer, RETRO_DEVICE_INDEX_ANALOG_LEFT, &analog_x, &analog_y );

				//
				// -- throttle

				int throttle_real;
				get_analog_axis( input_state_cb, iplayer,
					RETRO_DEVICE_INDEX_ANALOG_RIGHT,
					RETRO_DEVICE_ID_ANALOG_Y, &throttle_real );

				int16_t throttle;
				if ( input_mode[iplayer] & INPUT_MODE_MISSION_THROTTLE_LATCH )
				{
					// Use latched value
					throttle = input_throttle_latch[iplayer];
				}
				else
				{
					// Direct read
					throttle = throttle_real;
				}


				//
				// -- throttle latch switch

				{
					// Handle MODE button as a switch
					uint16_t prev = ( input_mode[iplayer] & INPUT_MODE_MISSION_THROTTLE_PREV );
					uint16_t held = 0;

					if (input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_mission_throttle_latch))
						held = INPUT_MODE_MISSION_THROTTLE_PREV;

					// Rising edge trigger?
					if ( !prev && held )
					{
						// Toggle 'state' bit: throttle latch enable/disable.
						input_mode[ iplayer ] ^= INPUT_MODE_MISSION_THROTTLE_LATCH;

						// Tell user
						if ( input_mode[iplayer] & INPUT_MODE_MISSION_THROTTLE_LATCH ) {
							log_cb( RETRO_LOG_INFO, "Controller %u: Latched Throttle at %d\n", (iplayer+1), throttle_real );
							input_throttle_latch[iplayer] = (int16_t)throttle_real;
						} else {
							log_cb( RETRO_LOG_INFO, "Controller %u: Remove Throttle Latch\n", (iplayer+1) );
						}
					}

					// Store held state in 'previous' bit.
					input_mode[ iplayer ] = ( input_mode[ iplayer ] & ~INPUT_MODE_MISSION_THROTTLE_PREV ) | held;
				}


				//
				// -- format input data

				// Convert analog values into direction values.
				uint16_t right = analog_x > 0 ?  analog_x : 0;
				uint16_t left  = analog_x < 0 ? -analog_x : 0;
				uint16_t down  = analog_y > 0 ?  analog_y : 0;
				uint16_t up    = analog_y < 0 ? -analog_y : 0;
				uint16_t th_up = throttle > 0 ?  throttle : 0;
				uint16_t th_dn = throttle < 0 ? -throttle : 0;

				p_input->u8[0x2] = 0; // todo: auto-fire controls.
				p_input->u8[0x3] = ((left  >> 0) & 0xff);
				p_input->u8[0x4] = ((left  >> 8) & 0xff);
				p_input->u8[0x5] = ((right >> 0) & 0xff);
				p_input->u8[0x6] = ((right >> 8) & 0xff);
				p_input->u8[0x7] = ((up    >> 0) & 0xff);
				p_input->u8[0x8] = ((up    >> 8) & 0xff);
				p_input->u8[0x9] = ((down  >> 0) & 0xff);
				p_input->u8[0xa] = ((down  >> 8) & 0xff);
				p_input->u8[0xb] = ((th_up >> 0) & 0xff);
				p_input->u8[0xc] = ((th_up >> 8) & 0xff);
				p_input->u8[0xd] = ((th_dn >> 0) & 0xff);
				p_input->u8[0xe] = ((th_dn >> 8) & 0xff);
			}

			break;

		case RETRO_DEVICE_SS_MISSION2:

			{
				//
				// -- mission stick buttons

				// input_map_mission is configured to quickly map libretro buttons to the correct bits for the Saturn.
				for ( int i = 0; i < INPUT_MAP_MISSION_SIZE; ++i )
					p_input->buttons |= input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_mission[ i ] ) ? ( 1 << i ) : 0;
				// .. the left trigger is a special case, there's a gap in the bits.
				p_input->buttons |=
					input_state_cb( iplayer, RETRO_DEVICE_JOYPAD, 0, input_map_mission_left_shoulder ) ? ( 1 << 11 ) : 0;

				//
				// -- analog sticks

				int analog1_x, analog1_y;
				int analog2_x, analog2_y;

				// Default - patent shows first stick on right side, second added on left
				// see: https://segaretro.org/images/a/a1/Patent_EP0745928A2.pdf
				get_analog_stick( input_state_cb, iplayer, RETRO_DEVICE_INDEX_ANALOG_RIGHT, &analog1_x, &analog1_y );
				get_analog_stick( input_state_cb, iplayer, RETRO_DEVICE_INDEX_ANALOG_LEFT, &analog2_x, &analog2_y );

				//
				// -- format input data

				// Convert analog values into direction values.
				uint16_t right1 = analog1_x > 0 ?  analog1_x : 0;
				uint16_t left1  = analog1_x < 0 ? -analog1_x : 0;
				uint16_t down1  = analog1_y > 0 ?  analog1_y : 0;
				uint16_t up1    = analog1_y < 0 ? -analog1_y : 0;

				uint16_t right2 = analog2_x > 0 ?  analog2_x : 0;
				uint16_t left2  = analog2_x < 0 ? -analog2_x : 0;
				uint16_t down2  = analog2_y > 0 ?  analog2_y : 0;
				uint16_t up2    = analog2_y < 0 ? -analog2_y : 0;

				p_input->u8[ 0x2] = 0; // todo: auto-fire controls.

				p_input->u8[ 0x3] = ((left1  >> 0) & 0xff);
				p_input->u8[ 0x4] = ((left1  >> 8) & 0xff);
				p_input->u8[ 0x5] = ((right1 >> 0) & 0xff);
				p_input->u8[ 0x6] = ((right1 >> 8) & 0xff);
				p_input->u8[ 0x7] = ((up1    >> 0) & 0xff);
				p_input->u8[ 0x8] = ((up1    >> 8) & 0xff);
				p_input->u8[ 0x9] = ((down1  >> 0) & 0xff);
				p_input->u8[ 0xa] = ((down1  >> 8) & 0xff);
				p_input->u8[ 0xb] = 0; // todo: throttle1
				p_input->u8[ 0xc] = 0;
				p_input->u8[ 0xd] = 0;
				p_input->u8[ 0xe] = 0;

				p_input->u8[ 0xf] = ((left2  >> 0) & 0xff);
				p_input->u8[0x10] = ((left2  >> 8) & 0xff);
				p_input->u8[0x11] = ((right2 >> 0) & 0xff);
				p_input->u8[0x12] = ((right2 >> 8) & 0xff);
				p_input->u8[0x13] = ((up2    >> 0) & 0xff);
				p_input->u8[0x14] = ((up2    >> 8) & 0xff);
				p_input->u8[0x15] = ((down2  >> 0) & 0xff);
				p_input->u8[0x16] = ((down2  >> 8) & 0xff);
				p_input->u8[0x17] = 0; // todo: throttle2
				p_input->u8[0x18] = 0;
				p_input->u8[0x19] = 0;
				p_input->u8[0x1a] = 0;
			}

			break;

		case RETRO_DEVICE_SS_GUN_JP:
		case RETRO_DEVICE_SS_GUN_US:

			{
				if ( setting_gun_input == SETTING_GUN_INPUT_POINTER ) {
					int gun_x, gun_y;
					int gun_x_raw, gun_y_raw;
					gun_x_raw = input_state_cb( iplayer, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
					gun_y_raw = input_state_cb( iplayer, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

					// .. scale into screen space:
					// NOTE: the scaling here is empirical-guesswork.
					// Tested at 352x240 (ntsc) and 352x256 (pal)

					const int scale_x = 21472;
					const int scale_y = geometry_height;
					const int offset_y = geometry_height - 240;

					int is_offscreen = 0;

					gun_x = ( ( gun_x_raw + 0x7fff ) * scale_x ) / (0x7fff << 1);
					gun_y = ( ( gun_y_raw + 0x7fff ) * scale_y ) / (0x7fff << 1) + offset_y;

					// Handle offscreen by checking corrected x and y values
					if ( gun_x == 0 || gun_y == 0 )
					{
						is_offscreen = 1;
						gun_x = -16384; // magic position to disable cross-hair drawing.
						gun_y = -16384;
					}

					// Touch sensitivity: Keep the gun position held for a fixed number of cycles after touch is released
					// because a very light touch can result in a misfire
					if ( pointer_cycles_after_released > 0 && pointer_cycles_after_released < POINTER_PRESSED_CYCLES ) {
						pointer_cycles_after_released++;
						p_input->gun_pos[ 0 ] = pointer_pressed_last_x;
						p_input->gun_pos[ 1 ] = pointer_pressed_last_y;
						return;
					}

					// trigger
					if ( input_state_cb( iplayer, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED ) )
					{
						pointer_pressed = 1;
						pointer_cycles_after_released = 0;
						pointer_pressed_last_x = gun_x;
						pointer_pressed_last_y = gun_y;
					} else if ( pointer_pressed ) {
						pointer_cycles_after_released++;
						pointer_pressed = 0;
						p_input->gun_pos[ 0 ] = pointer_pressed_last_x;
						p_input->gun_pos[ 1 ] = pointer_pressed_last_y;
						p_input->u8[4] &= ~0x1;
						return;
					}

					// position
					p_input->gun_pos[ 0 ] = gun_x;
					p_input->gun_pos[ 1 ] = gun_y;

					// buttons
					p_input->u8[ 4 ] = 0;

					// use multi-touch to support different button inputs:
					// 3-finger touch: START button
					// 2-finger touch: Reload
					// offscreen touch: Reload
					int touch_count = input_state_cb( iplayer, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_COUNT );
					if ( touch_count == 3 )
						p_input->u8[ 4 ] |= 0x2;
					else if ( touch_count == 2 )
						p_input->u8[ 4 ] |= 0x4;
					else if ( touch_count == 1 && is_offscreen )
						p_input->u8[ 4 ] |= 0x4;
					else if ( touch_count == 1 )
						p_input->u8[ 4 ] |= 0x1;

				} else {   // Lightgun input is default
					uint8_t shot_type;
					int gun_x, gun_y;
					int forced_reload = input_state_cb( iplayer, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD );

					// off-screen?
					if ( input_state_cb( iplayer, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN ) ||
						forced_reload ||
						geometry_height == 0 )
					{
						shot_type = 0x4; // off-screen shot

						gun_x = -16384; // magic position to disable cross-hair drawing.
						gun_y = -16384;
					}
					else
					{
						shot_type = 0x1; // on-screen shot

						int gun_x_raw = input_state_cb( iplayer, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X );
						int gun_y_raw = input_state_cb( iplayer, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y );

						// .. scale into screen space:
						// NOTE: the scaling here is empirical-guesswork.
						// Tested at 352x240 (ntsc) and 352x256 (pal)

						const int scale_x = 21472;
						const int scale_y = geometry_height;
						const int offset_y = geometry_height - 240;

						gun_x = ( ( gun_x_raw + 0x7fff ) * scale_x ) / (0x7fff << 1);
						gun_y = ( ( gun_y_raw + 0x7fff ) * scale_y ) / (0x7fff << 1) + offset_y;
					}

					// position
					p_input->gun_pos[ 0 ] = gun_x;
					p_input->gun_pos[ 1 ] = gun_y;

					// buttons
					p_input->u8[ 4 ] = 0;

					// trigger
					if ( input_state_cb( iplayer, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER ) || forced_reload )
						p_input->u8[ 4 ] |= shot_type;

					// start
					if ( input_state_cb( iplayer, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_START ) )
						p_input->u8[ 4 ] |= 0x2;

				}

			}

			break;

		} // switch ( input_type[ iplayer ] )

	} // for each player
}

// save state function for input
int input_StateAction( StateMem* sm, const unsigned load, const bool data_only )
{
	int success;

	SFORMAT StateRegs[] =
	{
		SFPTR16N( input_mode, MAX_CONTROLLERS, "pad-mode" ),
		SFPTR16N( input_throttle_latch, MAX_CONTROLLERS, "throttle-latch" ),
		SFEND
	};

	success = MDFNSS_StateAction( sm, load, data_only, StateRegs, "LIBRETRO-INPUT", false);

	// ok?
	return success;
}

//------------------------------------------------------------------------------
// Libretro Interface
//------------------------------------------------------------------------------

void retro_set_controller_port_device( unsigned in_port, unsigned device )
{
	if ( in_port < MAX_CONTROLLERS )
	{
		// Store input type
		input_type[ in_port ] = device;
		input_mode[ in_port ] = INPUT_MODE_DEFAULT;

		switch ( device )
		{

		case RETRO_DEVICE_NONE:
			log_cb( RETRO_LOG_INFO, "Controller %u: Unplugged\n", (in_port+1) );
			SMPC_SetInput( in_port, "none", (uint8*)&input_data[ in_port ] );
			break;

		case RETRO_DEVICE_JOYPAD:
		case RETRO_DEVICE_SS_PAD:
			log_cb( RETRO_LOG_INFO, "Controller %u: Control Pad\n", (in_port+1) );
			SMPC_SetInput( in_port, "gamepad", (uint8*)&input_data[ in_port ] );
			break;

		case RETRO_DEVICE_SS_3D_PAD:
			log_cb( RETRO_LOG_INFO, "Controller %u: 3D Control Pad\n", (in_port+1) );
			SMPC_SetInput( in_port, "3dpad", (uint8*)&input_data[ in_port ] );
			input_mode[ in_port ] = INPUT_MODE_DEFAULT_3D_PAD;
			break;

		case RETRO_DEVICE_SS_WHEEL:
			log_cb( RETRO_LOG_INFO, "Controller %u: Arcade Racer\n", (in_port+1) );
			SMPC_SetInput( in_port, "wheel", (uint8*)&input_data[ in_port ] );
			break;

		case RETRO_DEVICE_SS_MISSION:
			log_cb( RETRO_LOG_INFO, "Controller %u: Mission Stick\n", (in_port+1) );
			SMPC_SetInput( in_port, "mission", (uint8*)&input_data[ in_port ] );
			break;

		case RETRO_DEVICE_SS_MISSION2:
			log_cb( RETRO_LOG_INFO, "Controller %u: Dual Mission Sticks\n", (in_port+1) );
			SMPC_SetInput( in_port, "dmission", (uint8*)&input_data[ in_port ] );
			break;

		case RETRO_DEVICE_SS_MOUSE:
			log_cb( RETRO_LOG_INFO, "Controller %u: Mouse\n", (in_port+1) );
			SMPC_SetInput( in_port, "mouse", (uint8*)&input_data[ in_port ] );
			break;

		case RETRO_DEVICE_SS_GUN_US:
			log_cb( RETRO_LOG_INFO, "Controller %u: Stunner\n", (in_port+1) );
			SMPC_SetInput( in_port, "gun", (uint8*)&input_data[ in_port ] );
			break;

		case RETRO_DEVICE_SS_GUN_JP:
			log_cb( RETRO_LOG_INFO, "Controller %u: Virtua Gun\n", (in_port+1) );
			SMPC_SetInput( in_port, "gun", (uint8*)&input_data[ in_port ] );
			break;

		case RETRO_DEVICE_SS_TWINSTICK:
			log_cb( RETRO_LOG_INFO, "Controller %u: Twin-Stick\n", (in_port+1) );
			SMPC_SetInput( in_port, "gamepad", (uint8*)&input_data[ in_port ] );
			break;

		default:
			log_cb( RETRO_LOG_WARN, "Controller %u: Unsupported Device (%u)\n", (in_port+1), device );
			SMPC_SetInput( in_port, "none", (uint8*)&input_data[ in_port ] );
			break;

		}; // switch ( device )

	}; // valid port?
}

void input_multitap( int port, bool enabled )
{
	switch ( port )
	{
		case 1: // PORT 1
			if ( enabled != setting_multitap_port1 ) {
				setting_multitap_port1 = enabled;
				if ( setting_multitap_port1 ) {
					log_cb( RETRO_LOG_INFO, "Connected 6Player Adaptor to Port 1\n" );
				} else {
					log_cb( RETRO_LOG_INFO, "Removed 6Player Adaptor from Port 1\n" );
				}
				SMPC_SetMultitap( 0, setting_multitap_port1 );
			}
			break;

		case 2: // PORT 2
			if ( enabled != setting_multitap_port2 ) {
				setting_multitap_port2 = enabled;
				if ( setting_multitap_port2 ) {
					log_cb( RETRO_LOG_INFO, "Connected 6Player Adaptor to Port 2\n" );
				} else {
					log_cb( RETRO_LOG_INFO, "Removed 6Player Adaptor from Port 2\n" );
				}
				SMPC_SetMultitap( 1, setting_multitap_port2 );
			}
			break;

	}; // switch ( port )

	// Update players count
	players = 2;
	if ( setting_multitap_port1 ) {
		players += 5;
	}
	if ( setting_multitap_port2 ) {
		players += 5;
	}
	
	/*Tell front-end*/
	input_set_env( environ_cb );
}

//==============================================================================
