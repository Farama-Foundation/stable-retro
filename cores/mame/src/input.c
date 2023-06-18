/******************************************************************************

  input.c

  Handle input from the user - keyboard, joystick, etc.

******************************************************************************/

#include "driver.h"

#include <time.h>
#include <assert.h>

/***************************************************************************/
/* Codes */

/* Informations for every input code */
struct code_info {
	int memory; /* boolean memory */
	unsigned oscode; /* os dependant code */
	unsigned type; /* subtype: CODE_TYPE_KEYBOARD or CODE_TYPE_JOYSTICK */
};

/* Main code table, generic KEYCODE_*, JOYCODE_* are indexes in this table */
static struct code_info* code_map;

/* Size of the table */
static unsigned code_mac;

/* Create the code table */
int code_init(void)
{
	unsigned i;

	assert(	__code_key_first == 0
		&& __code_key_last + 1 == __code_joy_first
		&& __code_joy_last + 1 == __code_max );

	/* allocate */
	code_map = (struct code_info*)malloc( __code_max * sizeof(struct code_info) );
	if (!code_map)
		return -1;

	code_mac = 0;

	/* insert all known codes */
	for(i=0;i<__code_max;++i)
	{
		code_map[code_mac].memory = 0;
		code_map[code_mac].oscode = 0; /* not used */

		if (__code_key_first <= i && i <= __code_key_last)
			code_map[code_mac].type = CODE_TYPE_KEYBOARD;
		else if (__code_joy_first <= i && i <= __code_joy_last)
			code_map[code_mac].type = CODE_TYPE_JOYSTICK;
		else {
			/* never happen */
			assert(0);
			code_map[code_mac].type = CODE_TYPE_NONE;
		}
		++code_mac;
	}

	return 0;
}

/* Find the osd record of an oscode */
static INLINE const struct KeyboardInfo* internal_oscode_find_keyboard(unsigned oscode)
{
	const struct KeyboardInfo *keyinfo;
	keyinfo = osd_get_key_list();
	while (keyinfo->name)
	{
		if (keyinfo->code == oscode)
			return keyinfo;
		++keyinfo;
	}
	return 0;
}

static INLINE const struct JoystickInfo* internal_oscode_find_joystick(unsigned oscode)
{
	const struct JoystickInfo *joyinfo;
	joyinfo = osd_get_joy_list();
	/*log_cb(RETRO_LOG_DEBUG, "checking for oscode: %i\n", oscode);*/
	while (joyinfo->name)
	{
		/*log_cb(RETRO_LOG_DEBUG, "examining code: %i | name: %s\n", joyinfo->code, joyinfo->name);*/
		if (joyinfo->code == oscode)
			return joyinfo;
		++joyinfo;
	}
	/*log_cb(RETRO_LOG_DEBUG, "oscode not found\n");*/
	return 0;
}

/* Find a oscode in the table */
static int internal_oscode_find(unsigned oscode, unsigned type)
{
	unsigned i;
	const struct KeyboardInfo *keyinfo;
	const struct JoystickInfo *joyinfo;

	/* Search in the main table for an oscode */
	for(i=__code_max;i<code_mac;++i)
		if (code_map[i].type == type && code_map[i].oscode == oscode)
			return i;

	/* Search in the osd table for a standard code */
	switch (type)
	{
		case CODE_TYPE_KEYBOARD :
			keyinfo = internal_oscode_find_keyboard(oscode);
			if (keyinfo && keyinfo->standardcode != CODE_OTHER)
				return keyinfo->standardcode;
			break;
		case CODE_TYPE_JOYSTICK :
			joyinfo = internal_oscode_find_joystick(oscode);
			if (joyinfo && joyinfo->standardcode != CODE_OTHER)
				return joyinfo->standardcode;
			break;
	}

	/* oscode not found */
	return CODE_NONE;
}

int oscode_find(unsigned oscode, unsigned type)
{
  return internal_oscode_find(oscode, type);
}

/* Add a new oscode in the table */
static int internal_oscode_add(unsigned oscode, unsigned type)
{
	struct code_info* new_code_map;
	new_code_map = realloc( code_map, (code_mac+1) * sizeof(struct code_info) );
	if (new_code_map)
	{
		code_map = new_code_map;
		code_map[code_mac].memory = 0;
		code_map[code_mac].oscode = oscode;
		code_map[code_mac].type = type;
		return code_mac++;
	} else {
		return CODE_NONE;
        }
}

/* Find the osd record of a standard code */
static INLINE const struct KeyboardInfo* internal_code_find_keyboard(InputCode code)
{
	const struct KeyboardInfo *keyinfo;
	keyinfo = osd_get_key_list();

	assert( code < code_mac );

	if (code < __code_max)
	{
		while (keyinfo->name)
		{
			if (keyinfo->standardcode == code)
				return keyinfo;
			++keyinfo;
		}
	} else {
		while (keyinfo->name)
		{
			if (keyinfo->standardcode == CODE_OTHER && keyinfo->code == code_map[code].oscode)
				return keyinfo;
	      		++keyinfo;
		}
	}
	return 0;
}

static INLINE const struct JoystickInfo* internal_code_find_joystick(InputCode code)
{
	const struct JoystickInfo *joyinfo;
	joyinfo = osd_get_joy_list();

	assert( code < code_mac );

	if (code < __code_max)
	{
		while (joyinfo->name)
		{
			if (joyinfo->standardcode == code)
				return joyinfo;
			++joyinfo;
		}
	} else {
		while (joyinfo->name)
		{
			if (joyinfo->standardcode == CODE_OTHER && joyinfo->code == code_map[code].oscode)
				return joyinfo;
			++joyinfo;
		}
	}
	return 0;
}

/* Check if a code is pressed */
static int internal_code_pressed(InputCode code)
{
	const struct KeyboardInfo *keyinfo;
	const struct JoystickInfo *joyinfo;

	assert( code < code_mac );

	if (code < __code_max)
	{
		switch (code_map[code].type)
		{
			case CODE_TYPE_KEYBOARD :
				keyinfo = internal_code_find_keyboard(code);
				if (keyinfo)
					return osd_is_key_pressed(keyinfo->code);
				break;
			case CODE_TYPE_JOYSTICK :
				joyinfo = internal_code_find_joystick(code);
				if (joyinfo)
					return osd_is_joy_pressed(joyinfo->code);
				break;
		}
	} else {
		switch (code_map[code].type)
		{
			case CODE_TYPE_KEYBOARD :
				return osd_is_key_pressed(code_map[code].oscode);
			case CODE_TYPE_JOYSTICK :
				return osd_is_joy_pressed(code_map[code].oscode);
		}
	}
	return 0;
}

/* Return the name of the code */
static const char* internal_code_name(InputCode code)
{
	const struct KeyboardInfo *keyinfo;
	const struct JoystickInfo *joyinfo;

	assert( code < code_mac );

	switch (code_map[code].type)
	{
		case CODE_TYPE_KEYBOARD :
			keyinfo = internal_code_find_keyboard(code);
			if (keyinfo)
				return keyinfo->name;
			break;
		case CODE_TYPE_JOYSTICK :
			joyinfo = internal_code_find_joystick(code);
			if (joyinfo)
				return joyinfo->name;
			break;
	}
	return "n/a";
}

/* Update the code table */
void internal_code_update(void)
{
  const struct KeyboardInfo *keyinfo;
  const struct JoystickInfo *joyinfo;

  /* add only oscode because all standard codes are already present */

  keyinfo = osd_get_key_list();
  while (keyinfo->name)
  {
    if (keyinfo->standardcode == CODE_OTHER)
      if (internal_oscode_find(keyinfo->code,CODE_TYPE_KEYBOARD) == CODE_NONE)
        internal_oscode_add(keyinfo->code,CODE_TYPE_KEYBOARD);

    ++keyinfo;
  }

  joyinfo = osd_get_joy_list();
  while (joyinfo->name)
  {
    if (joyinfo->standardcode == CODE_OTHER)
      if (internal_oscode_find(joyinfo->code,CODE_TYPE_JOYSTICK)==CODE_NONE)
        internal_oscode_add(joyinfo->code,CODE_TYPE_JOYSTICK);

		++joyinfo;
	}
}

/* Delete the code table */
void code_close(void)
{
#if 0
	int i;
	log_cb(RETRO_LOG_ERROR, LOGPRE "List of OS dependant input codes:\n");
	for(i=__code_max;i<code_mac;++i)
		log_cb(RETRO_LOG_ERROR, LOGPRE "\tcode %d, oscode %d, %s, %s\n",i,code_map[i].oscode,code_map[i].type == CODE_TYPE_KEYBOARD ? "keyboard" : "joystick", internal_code_name(i));
#endif

	code_mac = 0;
	free(code_map);
	code_map = 0;
}

/***************************************************************************/
/* Save support */

/* Flags used for saving codes to file */
#define SAVECODE_FLAGS_TYPE_STANDARD 0x10000000 /* code */
#define SAVECODE_FLAGS_TYPE_KEYBOARD 0x20000000 /* keyboard oscode */
#define SAVECODE_FLAGS_TYPE_JOYSTICK 0x30000000 /* joystick oscode */
#define SAVECODE_FLAGS_TYPE_MASK     0xF0000000

/* Convert one key oscode to one standard code */
InputCode keyoscode_to_code(unsigned oscode)
{
	InputCode code;

	code = internal_oscode_find(oscode,CODE_TYPE_KEYBOARD);

	/* insert if missing */
	if (code == CODE_NONE)
		code = internal_oscode_add(oscode,CODE_TYPE_KEYBOARD);

	return code;
}

/* Convert one joystick oscode to one code */
InputCode joyoscode_to_code(unsigned oscode)
{
	InputCode code = internal_oscode_find(oscode,CODE_TYPE_JOYSTICK);

	/* insert if missing */
	if (code == CODE_NONE)
		code = internal_oscode_add(oscode,CODE_TYPE_JOYSTICK);

	return code;
}

/* Convert one saved code to one code */
InputCode savecode_to_code(unsigned savecode)
{
	unsigned type = savecode & SAVECODE_FLAGS_TYPE_MASK;
	InputCode code = savecode & ~SAVECODE_FLAGS_TYPE_MASK;

	switch (type)
	{
		case SAVECODE_FLAGS_TYPE_STANDARD :
			return code;
		case SAVECODE_FLAGS_TYPE_KEYBOARD :
			return keyoscode_to_code(code);
		case SAVECODE_FLAGS_TYPE_JOYSTICK :
			return joyoscode_to_code(code);
	}

	/* never happen */
	assert(0);
	return CODE_NONE;
}

/* Convert one code to one saved code */
unsigned code_to_savecode(InputCode code)
{
	if (code < __code_max || code >= code_mac)
               	/* if greather than code_mac is a special CODE like CODE_OR */
		return code | SAVECODE_FLAGS_TYPE_STANDARD;

	switch (code_map[code].type)
	{
		case CODE_TYPE_KEYBOARD : return code_map[code].oscode | SAVECODE_FLAGS_TYPE_KEYBOARD;
		case CODE_TYPE_JOYSTICK : return code_map[code].oscode | SAVECODE_FLAGS_TYPE_JOYSTICK;
	}

	/* never happen */
	assert(0);
	return 0;
}

/***************************************************************************/
/* Interface */

const char *code_name(InputCode code)
{
	if (code < code_mac)
		return internal_code_name(code);

	switch (code)
	{
		case CODE_NONE : return "None";
		case CODE_NOT : return "not";
		case CODE_OR : return "or";
	}

	return "n/a";
}

int code_pressed(InputCode code)
{
	int pressed;

	profiler_mark(PROFILER_INPUT);

	pressed = internal_code_pressed(code);

	profiler_mark(PROFILER_END);

	return pressed;
}

int code_pressed_memory(InputCode code)
{
	int pressed;

	profiler_mark(PROFILER_INPUT);

	pressed = internal_code_pressed(code);

	if (pressed)
	{
		if (code_map[code].memory == 0)
			code_map[code].memory = 1;
		else
			pressed = 0;
	} else
		code_map[code].memory = 0;

	profiler_mark(PROFILER_END);

	return pressed;
}

/* Report the pressure only if isn't already signaled with one of the */
/* functions code_memory and code_memory_repeat */
static int code_pressed_not_memorized(InputCode code)
{
	int pressed;

	profiler_mark(PROFILER_INPUT);

	pressed = internal_code_pressed(code);

	if (pressed)
	{
		if (code_map[code].memory != 0)
			pressed = 0;
	} else
		code_map[code].memory = 0;

	profiler_mark(PROFILER_END);

	return pressed;
}

int code_pressed_memory_repeat(InputCode code, int speed)
{
	static int counter;
	static int keydelay;
	int pressed;

	profiler_mark(PROFILER_INPUT);

	pressed = internal_code_pressed(code);

	if (pressed)
	{
		if (code_map[code].memory == 0)
		{
			code_map[code].memory = 1;
			keydelay = 3;
			counter = 0;
		}
		else if (++counter > keydelay * speed * Machine->drv->frames_per_second / 60)
		{
			keydelay = 1;
			counter = 0;
		} else
			pressed = 0;
	} else
		code_map[code].memory = 0;

	profiler_mark(PROFILER_END);

	return pressed;
}

InputCode code_read_async(void)
{
	unsigned i;

	profiler_mark(PROFILER_INPUT);

	/* update the table */
	internal_code_update();

	for(i=0;i<code_mac;++i)
		if (code_pressed_memory(i))
			return i;

	profiler_mark(PROFILER_END);

	return CODE_NONE;
}

/* returns the numerical value of a typed hex digit, or -1 if none */
INT8 code_read_hex_async(void)
{
	unsigned i;

	profiler_mark(PROFILER_INPUT);

	/* update the table */
	internal_code_update();

	for(i=0;i<code_mac;++i)
		if (code_pressed_memory(i))
		{
			if ((i >= KEYCODE_A) && (i <= KEYCODE_F))
				return i - KEYCODE_A + 10;
			else if ((i >= KEYCODE_0) && (i <= KEYCODE_9))
				return i - KEYCODE_0;
			else
				return -1;
		}

	profiler_mark(PROFILER_END);

	return -1;
}

/***************************************************************************/
/* Sequences */

void seq_set_0(InputSeq* a)
{
	int j;
	for(j=0;j<SEQ_MAX;++j)
		(*a)[j] = CODE_NONE;
}

void seq_set_1(InputSeq* a, InputCode code)
{
	int j;
	(*a)[0] = code;
	for(j=1;j<SEQ_MAX;++j)
		(*a)[j] = CODE_NONE;
}

void seq_set_2(InputSeq* a, InputCode code1, InputCode code2)
{
	int j;
	(*a)[0] = code1;
	(*a)[1] = code2;
	for(j=2;j<SEQ_MAX;++j)
		(*a)[j] = CODE_NONE;
}

void seq_set_3(InputSeq* a, InputCode code1, InputCode code2, InputCode code3)
{
	int j;
	(*a)[0] = code1;
	(*a)[1] = code2;
	(*a)[2] = code3;
	for(j=3;j<SEQ_MAX;++j)
		(*a)[j] = CODE_NONE;
}

void seq_set_4(InputSeq* a, InputCode code1, InputCode code2, InputCode code3, InputCode code4)
{
	int j;
	(*a)[0] = code1;
	(*a)[1] = code2;
	(*a)[2] = code3;
	(*a)[3] = code4;
	for(j=4;j<SEQ_MAX;++j)
		(*a)[j] = CODE_NONE;
}

void seq_set_5(InputSeq* a, InputCode code1, InputCode code2, InputCode code3, InputCode code4, InputCode code5)
{
	int j;
	(*a)[0] = code1;
	(*a)[1] = code2;
	(*a)[2] = code3;
	(*a)[3] = code4;
	(*a)[4] = code5;
	for(j=5;j<SEQ_MAX;++j)
		(*a)[j] = CODE_NONE;
}

void seq_set_6(InputSeq* a, InputCode code1, InputCode code2, InputCode code3, InputCode code4, InputCode code5, InputCode code6)
{
	int j;
	(*a)[0] = code1;
	(*a)[1] = code2;
	(*a)[2] = code3;
	(*a)[3] = code4;
	(*a)[4] = code5;
	(*a)[5] = code6;
	for(j=6;j<SEQ_MAX;++j)
		(*a)[j] = CODE_NONE;
}

void seq_set_7(InputSeq* a, InputCode code1, InputCode code2, InputCode code3, InputCode code4, InputCode code5, InputCode code6, InputCode code7)
{
	int j;
	(*a)[0] = code1;
	(*a)[1] = code2;
	(*a)[2] = code3;
	(*a)[3] = code4;
	(*a)[4] = code5;
	(*a)[5] = code6;
	(*a)[6] = code7;
	for(j=7;j<SEQ_MAX;++j)
		(*a)[j] = CODE_NONE;
}

void seq_copy(InputSeq* a, InputSeq* b)
{
	int j;
	for(j=0;j<SEQ_MAX;++j)
		(*a)[j] = (*b)[j];
}

int seq_cmp(InputSeq* a, InputSeq* b)
{
	int j;
	for(j=0;j<SEQ_MAX;++j)
		if ((*a)[j] != (*b)[j])
			return -1;
	return 0;
}

void seq_name(InputSeq* code, char* buffer, unsigned max)
{
	int j;
	char* dest = buffer;
	for(j=0;j<SEQ_MAX;++j)
	{
		const char* name;

		/* this reduces those pesky "blah or n/a" constructs */
		if (((*code)[j]==CODE_OR) && (j+1>=SEQ_MAX || !strcmp(code_name((*code)[j+1]), "n/a")))
		{
			++j;
			continue;
		}

		if ((*code)[j]==CODE_NONE)
			break;

		if (j && 1 + 1 <= max)
		{
			*dest = ' ';
			dest += 1;
			max -= 1;
		}

		name = code_name((*code)[j]);
		if (!name)
			break;

		if (strlen(name) + 1 <= max)
		{
			strcpy(dest,name);
			dest += strlen(name);
			max -= strlen(name);
		}
	}

	if (dest == buffer && 4 + 1 <= max)
		strcpy(dest,"None");
	else
		*dest = 0;
}

int seq_pressed(InputSeq* code)
{
	int j;
	int res = 1;
	int invert = 0;
	int count = 0;

	for(j=0;j<SEQ_MAX;++j)
	{
		switch ((*code)[j])
		{
			case CODE_NONE :
				return res && count;
			case CODE_OR :
				if (res && count)
					return 1;
				res = 1;
				count = 0;
				break;
			case CODE_NOT :
				invert = !invert;
				break;
			default:
				if (res)
				{
					int pressed = code_pressed_not_memorized((*code)[j]);
					if ((pressed != 0) == invert)
						res = 0;
				}
				invert = 0;
				++count;
		}
	}
	return res && count;
}

/* Static informations used in key/joy recording */
static InputCode record_seq[SEQ_MAX]; /* buffer for key recording */
static int record_count; /* number of key/joy press recorded */
static clock_t record_last; /* time of last key/joy press */

/* Windows out of spec workaround */
#if defined WIN32 || (defined _WIN64) || (defined _WINDOWS)
#define RECORD_TIME (CLOCKS_PER_SEC*2/3) /* max time between key press */
#else
#define RECORD_TIME (CLOCKS_PER_SEC/8) /* max time between key press */
#endif

/* Start a sequence recording */
void seq_read_async_start(void)
{
	unsigned i;

	record_count = 0;
	record_last = clock();

	/* reset code memory, otherwise this memory may interferes with the input memory */
	for(i=0;i<code_mac;++i)
		code_map[i].memory = 1;
}

/* Check that almost one key/joy must be pressed */
static int seq_valid(InputSeq* seq)
{
	int j;
	int positive = 0;
	int pred_not = 0;
	int operand = 0;
	for(j=0;j<SEQ_MAX;++j)
	{
		switch ((*seq)[j])
		{
			case CODE_NONE :
				break;
			case CODE_OR :
				if (!operand || !positive)
					return 0;
				pred_not = 0;
				positive = 0;
				operand = 0;
				break;
			case CODE_NOT :
				if (pred_not)
					return 0;
				pred_not = !pred_not;
				operand = 0;
				break;
			default:
				if (!pred_not)
					positive = 1;
				pred_not = 0;
				operand = 1;
				break;
		}
	}
	return positive && operand;
}

/* Record a key/joy sequence
	return <0 if more input is needed
	return ==0 if sequence succesfully recorded
	return >0 if aborted
*/
int seq_read_async(InputSeq* seq, int first)
{
	InputCode newkey;

    /* Ignore UI_CANCEL when rebinding as you may want to rebind
       the key/button that UI_CANCEL is bound to, especially in RETROPAD-only
       environments.
     */
	/*if (input_ui_pressed(IPT_UI_CANCEL))*/
	/*	return 1;*/

	if (record_count == SEQ_MAX
		|| (record_count > 0 && clock() > record_last + RECORD_TIME))	{
		int k = 0;
		if (!first)
		{
			/* search the first space free */
			while (k < SEQ_MAX && (*seq)[k] != CODE_NONE)
				++k;
		}

		/* if no space restart */
		if (k + record_count + (k!=0) > SEQ_MAX)
			k = 0;

		/* insert */
		if (k + record_count + (k!=0) <= SEQ_MAX)
		{
			int j;
			if (k!=0)
				(*seq)[k++] = CODE_OR;
			for(j=0;j<record_count;++j,++k)
				(*seq)[k] = record_seq[j];
		}
		/* fill to end */
		while (k < SEQ_MAX)
		{
			(*seq)[k] = CODE_NONE;
			++k;
		}

		if (!seq_valid(seq))
			seq_set_1(seq,CODE_NONE);

		return 0;
	}

	newkey = code_read_async();

	if (newkey != CODE_NONE)
	{
		/* if code is duplicate negate the code */
		if (record_count && newkey == record_seq[record_count-1])
			record_seq[record_count-1] = CODE_NOT;

		record_seq[record_count++] = newkey;
		record_last = clock();
	}

	return -1;
}

/***************************************************************************/
/* input ui */

/* Static buffer for memory input */
struct ui_info {
	int memory;
};

static struct ui_info ui_map[__ipt_max];

int input_ui_pressed(int code)
{
	int pressed;

	profiler_mark(PROFILER_INPUT);

	pressed = seq_pressed(input_port_type_seq(code));

	if (pressed)
	{
		if (ui_map[code].memory == 0)
		{
			ui_map[code].memory = 1;
		} else
			pressed = 0;
	} else
		ui_map[code].memory = 0;

	profiler_mark(PROFILER_END);

	return pressed;
}

int input_ui_pressed_repeat(int code,int speed)
{
	static int counter,inputdelay;
	int pressed;

	profiler_mark(PROFILER_INPUT);

	pressed = seq_pressed(input_port_type_seq(code));

	if (pressed)
	{
		if (ui_map[code].memory == 0)
		{
			ui_map[code].memory = 1;
			inputdelay = 3;
			counter = 0;
		}
		else if (++counter > inputdelay * speed * Machine->drv->frames_per_second / 60)
		{
			inputdelay = 1;
			counter = 0;
		} else
			pressed = 0;
	} else
		ui_map[code].memory = 0;

	profiler_mark(PROFILER_END);

	return pressed;
}

int is_joystick_axis_code(unsigned code)
{
	const struct JoystickInfo *joyinfo;

	assert( code < code_mac );

	if (code_map[code].type == CODE_TYPE_JOYSTICK)
	{
		if (code < __code_max)
		{
			joyinfo = internal_code_find_joystick(code);
			if (joyinfo)
				return osd_is_joystick_axis_code(joyinfo->code);
		}
		else
		{
			return osd_is_joystick_axis_code(code_map[code].oscode);
		}
	}

	return 0;
}

int return_os_joycode(InputCode code)
{
	const struct JoystickInfo *joyinfo;

	assert( code < code_mac );

	if (code < __code_max)
	{
		if (code_map[code].type == CODE_TYPE_JOYSTICK)
		{
			joyinfo = internal_code_find_joystick(code);
			if (joyinfo)
				return joyinfo->code;
		}
	} else {
		if (code_map[code].type == CODE_TYPE_JOYSTICK)
		{
			return code_map[code].oscode;
		}
	}
	return 0;
}
