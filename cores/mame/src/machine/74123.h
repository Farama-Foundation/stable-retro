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

#ifndef TTL74123_H
#define TTL74123_H

#define MAX_TTL74123 4

/* The interface structure */
struct TTL74123_interface {
	double res;
	double cap;
	void (*output_changed_cb)(void);
};


void TTL74123_unconfig(void);
void TTL74123_config(int which, const struct TTL74123_interface *intf);

void TTL74123_trigger_w(int which, int data);
void TTL74123_trigger_comp_w(int which, int data);
void TTL74123_reset_comp_w(int which, int data);
int  TTL74123_output_r(int which);
int  TTL74123_output_comp_r(int which);

#endif
