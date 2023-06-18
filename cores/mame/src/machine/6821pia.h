/**********************************************************************

	Motorola 6821 PIA interface and emulation

	This function emulates all the functionality of up to 4 M6821
	peripheral interface adapters.

**********************************************************************/

#ifndef PIA_6821
#define PIA_6821


#define MAX_PIA 8


/* this is the standard ordering of the registers */
/* alternate ordering swaps registers 1 and 2 */
#define	PIA_DDRA	0
#define	PIA_CTLA	1
#define	PIA_DDRB	2
#define	PIA_CTLB	3

/* PIA addressing modes */
#define PIA_STANDARD_ORDERING		0
#define PIA_ALTERNATE_ORDERING		1

struct pia6821_interface
{
	mem_read_handler in_a_func;
	mem_read_handler in_b_func;
	mem_read_handler in_ca1_func;
	mem_read_handler in_cb1_func;
	mem_read_handler in_ca2_func;
	mem_read_handler in_cb2_func;
	mem_write_handler out_a_func;
	mem_write_handler out_b_func;
	mem_write_handler out_ca2_func;
	mem_write_handler out_cb2_func;
	void (*irq_a_func)(int state);
	void (*irq_b_func)(int state);
};

#ifdef __cplusplus
extern "C" {
#endif

void pia_init(int count);
void pia_unconfig(void);
void pia_config(int which, int addressing, const struct pia6821_interface *intf);
void pia_reset(void);
int pia_read(int which, int offset);
void pia_write(int which, int offset, int data);
void pia_set_input_a(int which, int data);
void pia_set_input_ca1(int which, int data);
void pia_set_input_ca2(int which, int data);
void pia_set_input_b(int which, int data);
void pia_set_input_cb1(int which, int data);
void pia_set_input_cb2(int which, int data);

#define PIA_UNUSED_VAL(x) ((mem_read_handler)(x+1))
/******************* Standard 8-bit CPU interfaces, D0-D7 *******************/

READ_HANDLER( pia_0_r );
READ_HANDLER( pia_1_r );
READ_HANDLER( pia_2_r );
READ_HANDLER( pia_3_r );
READ_HANDLER( pia_4_r );
READ_HANDLER( pia_5_r );
READ_HANDLER( pia_6_r );
READ_HANDLER( pia_7_r );

WRITE_HANDLER( pia_0_w );
WRITE_HANDLER( pia_1_w );
WRITE_HANDLER( pia_2_w );
WRITE_HANDLER( pia_3_w );
WRITE_HANDLER( pia_4_w );
WRITE_HANDLER( pia_5_w );
WRITE_HANDLER( pia_6_w );
WRITE_HANDLER( pia_7_w );

/******************* Standard 16-bit CPU interfaces, D0-D7 *******************/

READ16_HANDLER( pia_0_lsb_r );
READ16_HANDLER( pia_1_lsb_r );
READ16_HANDLER( pia_2_lsb_r );
READ16_HANDLER( pia_3_lsb_r );
READ16_HANDLER( pia_4_lsb_r );
READ16_HANDLER( pia_5_lsb_r );
READ16_HANDLER( pia_6_lsb_r );
READ16_HANDLER( pia_7_lsb_r );

WRITE16_HANDLER( pia_0_lsb_w );
WRITE16_HANDLER( pia_1_lsb_w );
WRITE16_HANDLER( pia_2_lsb_w );
WRITE16_HANDLER( pia_3_lsb_w );
WRITE16_HANDLER( pia_4_lsb_w );
WRITE16_HANDLER( pia_5_lsb_w );
WRITE16_HANDLER( pia_6_lsb_w );
WRITE16_HANDLER( pia_7_lsb_w );

/******************* Standard 16-bit CPU interfaces, D8-D15 *******************/

READ16_HANDLER( pia_0_msb_r );
READ16_HANDLER( pia_1_msb_r );
READ16_HANDLER( pia_2_msb_r );
READ16_HANDLER( pia_3_msb_r );
READ16_HANDLER( pia_4_msb_r );
READ16_HANDLER( pia_5_msb_r );
READ16_HANDLER( pia_6_msb_r );
READ16_HANDLER( pia_7_msb_r );

WRITE16_HANDLER( pia_0_msb_w );
WRITE16_HANDLER( pia_1_msb_w );
WRITE16_HANDLER( pia_2_msb_w );
WRITE16_HANDLER( pia_3_msb_w );
WRITE16_HANDLER( pia_4_msb_w );
WRITE16_HANDLER( pia_5_msb_w );
WRITE16_HANDLER( pia_6_msb_w );
WRITE16_HANDLER( pia_7_msb_w );

/******************* 8-bit A/B port interfaces *******************/

WRITE_HANDLER( pia_0_porta_w );
WRITE_HANDLER( pia_1_porta_w );
WRITE_HANDLER( pia_2_porta_w );
WRITE_HANDLER( pia_3_porta_w );
WRITE_HANDLER( pia_4_porta_w );
WRITE_HANDLER( pia_5_porta_w );
WRITE_HANDLER( pia_6_porta_w );
WRITE_HANDLER( pia_7_porta_w );

WRITE_HANDLER( pia_0_portb_w );
WRITE_HANDLER( pia_1_portb_w );
WRITE_HANDLER( pia_2_portb_w );
WRITE_HANDLER( pia_3_portb_w );
WRITE_HANDLER( pia_4_portb_w );
WRITE_HANDLER( pia_5_portb_w );
WRITE_HANDLER( pia_6_portb_w );
WRITE_HANDLER( pia_7_portb_w );

READ_HANDLER( pia_0_porta_r );
READ_HANDLER( pia_1_porta_r );
READ_HANDLER( pia_2_porta_r );
READ_HANDLER( pia_3_porta_r );
READ_HANDLER( pia_4_porta_r );
READ_HANDLER( pia_5_porta_r );
READ_HANDLER( pia_6_porta_r );
READ_HANDLER( pia_7_porta_r );

READ_HANDLER( pia_0_portb_r );
READ_HANDLER( pia_1_portb_r );
READ_HANDLER( pia_2_portb_r );
READ_HANDLER( pia_3_portb_r );
READ_HANDLER( pia_4_portb_r );
READ_HANDLER( pia_5_portb_r );
READ_HANDLER( pia_6_portb_r );
READ_HANDLER( pia_7_portb_r );

/******************* 1-bit CA1/CA2/CB1/CB2 port interfaces *******************/

WRITE_HANDLER( pia_0_ca1_w );
WRITE_HANDLER( pia_1_ca1_w );
WRITE_HANDLER( pia_2_ca1_w );
WRITE_HANDLER( pia_3_ca1_w );
WRITE_HANDLER( pia_4_ca1_w );
WRITE_HANDLER( pia_5_ca1_w );
WRITE_HANDLER( pia_6_ca1_w );
WRITE_HANDLER( pia_7_ca1_w );
WRITE_HANDLER( pia_0_ca2_w );
WRITE_HANDLER( pia_1_ca2_w );
WRITE_HANDLER( pia_2_ca2_w );
WRITE_HANDLER( pia_3_ca2_w );
WRITE_HANDLER( pia_4_ca2_w );
WRITE_HANDLER( pia_5_ca2_w );
WRITE_HANDLER( pia_6_ca2_w );
WRITE_HANDLER( pia_7_ca2_w );

WRITE_HANDLER( pia_0_cb1_w );
WRITE_HANDLER( pia_1_cb1_w );
WRITE_HANDLER( pia_2_cb1_w );
WRITE_HANDLER( pia_3_cb1_w );
WRITE_HANDLER( pia_4_cb1_w );
WRITE_HANDLER( pia_5_cb1_w );
WRITE_HANDLER( pia_6_cb1_w );
WRITE_HANDLER( pia_7_cb1_w );
WRITE_HANDLER( pia_0_cb2_w );
WRITE_HANDLER( pia_1_cb2_w );
WRITE_HANDLER( pia_2_cb2_w );
WRITE_HANDLER( pia_3_cb2_w );
WRITE_HANDLER( pia_4_cb2_w );
WRITE_HANDLER( pia_5_cb2_w );
WRITE_HANDLER( pia_6_cb2_w );
WRITE_HANDLER( pia_7_cb2_w );

READ_HANDLER( pia_0_ca1_r );
READ_HANDLER( pia_1_ca1_r );
READ_HANDLER( pia_2_ca1_r );
READ_HANDLER( pia_3_ca1_r );
READ_HANDLER( pia_4_ca1_r );
READ_HANDLER( pia_5_ca1_r );
READ_HANDLER( pia_6_ca1_r );
READ_HANDLER( pia_7_ca1_r );
READ_HANDLER( pia_0_ca2_r );
READ_HANDLER( pia_1_ca2_r );
READ_HANDLER( pia_2_ca2_r );
READ_HANDLER( pia_3_ca2_r );
READ_HANDLER( pia_4_ca2_r );
READ_HANDLER( pia_5_ca2_r );
READ_HANDLER( pia_6_ca2_r );
READ_HANDLER( pia_7_ca2_r );

READ_HANDLER( pia_0_cb1_r );
READ_HANDLER( pia_1_cb1_r );
READ_HANDLER( pia_2_cb1_r );
READ_HANDLER( pia_3_cb1_r );
READ_HANDLER( pia_4_cb1_r );
READ_HANDLER( pia_5_cb1_r );
READ_HANDLER( pia_6_cb1_r );
READ_HANDLER( pia_7_cb1_r );
READ_HANDLER( pia_0_cb2_r );
READ_HANDLER( pia_1_cb2_r );
READ_HANDLER( pia_2_cb2_r );
READ_HANDLER( pia_3_cb2_r );
READ_HANDLER( pia_4_cb2_r );
READ_HANDLER( pia_5_cb2_r );
READ_HANDLER( pia_6_cb2_r );
READ_HANDLER( pia_7_cb2_r );

#ifdef __cplusplus
}
#endif

#endif
