/***************************************************************************

  RIOT 6532 emulation

***************************************************************************/

#include "memory.h"


struct R6532interface
{
	mem_read_handler portA_r;
	mem_read_handler portB_r;

	mem_write_handler portA_w;
	mem_write_handler portB_w;
};


extern void r6532_init(int n, const struct R6532interface* RI);

extern READ_HANDLER( r6532_0_r );
extern READ_HANDLER( r6532_1_r );

extern WRITE_HANDLER( r6532_0_w );
extern WRITE_HANDLER( r6532_1_w );
