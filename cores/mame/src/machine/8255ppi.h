#ifndef _8255PPI_H_
#define _8255PPI_H_

#define MAX_8255 8

typedef struct
{
	int num;							 /* number of PPIs to emulate */
	mem_read_handler portAread[MAX_8255];
	mem_read_handler portBread[MAX_8255];
	mem_read_handler portCread[MAX_8255];
	mem_write_handler portAwrite[MAX_8255];
	mem_write_handler portBwrite[MAX_8255];
	mem_write_handler portCwrite[MAX_8255];
} ppi8255_interface;


/* Init */
void ppi8255_init( ppi8255_interface *intfce);

/* Read/Write */
int ppi8255_r ( int which, int offset );
void ppi8255_w( int which, int offset, int data );

void ppi8255_set_portAread( int which, mem_read_handler portAread);
void ppi8255_set_portBread( int which, mem_read_handler portBread);
void ppi8255_set_portCread( int which, mem_read_handler portCread);

void ppi8255_set_portAwrite( int which, mem_write_handler portAwrite);
void ppi8255_set_portBwrite( int which, mem_write_handler portBwrite);
void ppi8255_set_portCwrite( int which, mem_write_handler portCwrite);

/* Helpers */
READ_HANDLER( ppi8255_0_r );
READ_HANDLER( ppi8255_1_r );
READ_HANDLER( ppi8255_2_r );
READ_HANDLER( ppi8255_3_r );
READ_HANDLER( ppi8255_4_r );
READ_HANDLER( ppi8255_5_r );
READ_HANDLER( ppi8255_6_r );
READ_HANDLER( ppi8255_7_r );
WRITE_HANDLER( ppi8255_0_w );
WRITE_HANDLER( ppi8255_1_w );
WRITE_HANDLER( ppi8255_2_w );
WRITE_HANDLER( ppi8255_3_w );
WRITE_HANDLER( ppi8255_4_w );
WRITE_HANDLER( ppi8255_5_w );
WRITE_HANDLER( ppi8255_6_w );
WRITE_HANDLER( ppi8255_7_w );
#endif
