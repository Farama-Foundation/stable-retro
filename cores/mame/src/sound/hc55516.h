#ifndef HC55516_H
#define HC55516_H

#define MAX_HC55516		4

struct hc55516_interface
{
	int num;
	int volume[MAX_HC55516];
};

int hc55516_sh_start(const struct MachineSound *msound);

/* sets the databit (0 or 1) */
void hc55516_digit_w(int num, int data);

/* sets the clock state (0 or 1, clocked on the rising edge) */
void hc55516_clock_w(int num, int state);

/* clears or sets the clock state */
void hc55516_clock_clear_w(int num, int data);
void hc55516_clock_set_w(int num, int data);

/* clears the clock state and sets the databit */
void hc55516_digit_clock_clear_w(int num, int data);

WRITE_HANDLER( hc55516_0_digit_w );
WRITE_HANDLER( hc55516_0_clock_w );
WRITE_HANDLER( hc55516_0_clock_clear_w );
WRITE_HANDLER( hc55516_0_clock_set_w );
WRITE_HANDLER( hc55516_0_digit_clock_clear_w );

WRITE_HANDLER( hc55516_1_digit_w );
WRITE_HANDLER( hc55516_1_clock_w );
WRITE_HANDLER( hc55516_1_clock_clear_w );
WRITE_HANDLER( hc55516_1_clock_set_w );
WRITE_HANDLER( hc55516_1_digit_clock_clear_w );


#endif
