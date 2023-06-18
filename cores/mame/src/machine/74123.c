/*****************************************************************************

  74123 monoflop emulator - there are 2 monoflops per chips

  74123 pins and assigned interface variables/functions

	TTL74123_trigger_comp_w	[ 1] /1TR		   VCC [16]
	TTL74123_trigger_w		[ 2]  1TR		1RCext [15]
  	TTL74123_reset_comp_w	[ 3] /1RST		 1Cext [14]
	TTL74123_output_comp_r	[ 4] /1Q		    1Q [13] TTL74123_output_r
	TTL74123_output_r		[ 5]  2Q	       /2Q [12] TTL74123_output_comp_r
		 					[ 6]  2Cext	     /2RST [11] TTL74123_reset_comp_w
		 					[ 7]  2RCext       2TR [10] TTL74123_trigger_w
   							[ 8]  GND	      /2TR [9]  TTL74123_trigger_comp_w

	All resistor values in Ohms.
	All capacitor values in Farads.

	Truth table:
	R	A	B | Q  /Q
	----------|-------
	L	X	X | L	H
	X	H	X | L	H
	X	X	L | L	H
	H	L  _- |_-_ -_-
	H  -_	H |_-_ -_-
	_-	L	H |_-_ -_-
	------------------
	A	= trigger_comp
	B	= trigger
	R	= reset_comp
	L	= lo (0)
	H	= hi (1)
	X	= any state
	_-	= raising edge
	-_	= falling edge
	_-_ = positive pulse
	-_- = negative pulse

 *****************************************************************************/

#include "driver.h"
#include "machine/74123.h"

struct TTL74123 {
	const struct TTL74123_interface *intf;
	int trigger;			/* pin 2/10 */
	int trigger_comp;		/* pin 1/9 */
	int reset_comp;			/* pin 3/11 */
	int output;				/* pin 13/5 */
	void *timer;
	int timer_active;
};

static struct TTL74123 chip[MAX_TTL74123];


static void set_output(int which, int data)
{
	chip[which].output = data;
	chip[which].intf->output_changed_cb();
}


static void clear_callback(int which)
{
	struct TTL74123 *c = chip + which;

    c->timer_active = 0;
	set_output(which, 0);
}


void TTL74123_config(int which, const struct TTL74123_interface *intf)
{
	if (which >= MAX_TTL74123) return;

	chip[which].intf = intf;

	/* all inputs are open first */
    chip[which].trigger = 1;
	chip[which].trigger_comp = 1;
	chip[which].reset_comp = 1;
	chip[which].timer = timer_alloc(clear_callback);
	set_output(which, 1);
}


void TTL74123_unconfig(void)
{
	memset(&chip, 0, sizeof(chip));
}


#define CHECK_TRIGGER(COND) 													\
	{																			\
		if (COND)																\
		{																		\
			double duration = TIME_IN_SEC(0.68 * c->intf->res * c->intf->cap);	\
			if (!c->timer_active) set_output(which, 1);							\
			timer_adjust(c->timer, duration, which, 0);							\
			c->timer_active = 1;												\
		}																		\
	}

#define RESET																	\
	if (c->timer_active)														\
		timer_adjust(c->timer, TIME_NOW, which, 0);								\


void TTL74123_trigger_w(int which, int data)
{
	struct TTL74123 *c = chip + which;

	/* trigger_comp=lo and rising edge on trigger (while reset_comp is hi) */
	if (data)
		CHECK_TRIGGER(!c->trigger_comp && !c->trigger && c->reset_comp)
	else
		RESET

	c->trigger = data;
}


void TTL74123_trigger_comp_w(int which, int data)
{
	struct TTL74123 *c = chip + which;

	/* trigger=hi and falling edge on trigger_comp (while reset_comp is hi) */
	if (!data)
		CHECK_TRIGGER(c->trigger && c->trigger_comp && c->reset_comp)
	else
		RESET

	c->trigger_comp = data;
}


void TTL74123_reset_comp_w(int which, int data)
{
	struct TTL74123 *c = chip + which;

	/* trigger=hi, trigger_comp=lo and rising edge on reset_comp */
	if (data)
    	CHECK_TRIGGER(c->trigger && !c->trigger_comp && !c->reset_comp)
	else
		RESET

	c->reset_comp = data;
}


int TTL74123_output_r(int which)
{
	return chip[which].output;
}


int TTL74123_output_comp_r(int which)
{
	return !chip[which].output;
}
