#ifndef _8254PIT_H_
#define _8254PIT_H_

#define MAX_8254 2

typedef struct
{
    int num;                             /* number of PITs to emulate */
    int baseclock1[MAX_8254];            /* timer clock */
    int baseclock2[MAX_8254];            /* timer clock */
    int baseclock3[MAX_8254];            /* timer clock */
    void (*out[MAX_8254])(int which);    /* timer callback */
} pit8254_interface;

void pit8254_init (pit8254_interface *intf);

void pit8254_w (int which, int offset, int data);
WRITE_HANDLER( pit8254_0_w );
WRITE_HANDLER( pit8254_0_counter1_w );
WRITE_HANDLER( pit8254_0_counter2_w );
WRITE_HANDLER( pit8254_0_counter3_w );
WRITE_HANDLER( pit8254_0_control_w );

WRITE_HANDLER( pit8254_1_w );
WRITE_HANDLER( pit8254_1_counter1_w );
WRITE_HANDLER( pit8254_1_counter2_w );
WRITE_HANDLER( pit8254_1_counter3_w );
WRITE_HANDLER( pit8254_1_control_w );

int pit8254_r (int which, int offset);
READ_HANDLER( pit8254_0_r );
READ_HANDLER( pit8254_0_counter1_r );
READ_HANDLER( pit8254_0_counter2_r );
READ_HANDLER( pit8254_0_counter3_r );
READ_HANDLER( pit8254_0_control_r );
READ_HANDLER( pit8254_1_r );
READ_HANDLER( pit8254_1_counter1_r );
READ_HANDLER( pit8254_1_counter2_r );
READ_HANDLER( pit8254_1_counter3_r );
READ_HANDLER( pit8254_1_control_r );

#endif
