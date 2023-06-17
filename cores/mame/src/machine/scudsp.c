#include "driver.h"
#include "machine/scudsp.h"

/*SCU DSP functions */
/*V0.03*/

/* 
031211: (Mariusz Wojcieszek changes)
- result of ALU command is stored into ALU register 
- X-Bus command: MOV [s],X can be executed in parallel to other X-Bus commands
- Y-Bus command: MOV [s],Y can be executed in parallel to other Y-Bus commands
- Jump and LPS/BTM support: 
	jump addresses are absolute, 
	prefetched instructions are executed before jump is taken
- after each instruction, X and Y is multiplied and contents are loaded into MUL register
- fixed RL8
- fixed MVI
- flags computation in MVI and JMP is partly guessed (because of errors in docs)
- added reading DSP mem from SH2 side
- overworked disassembler

 TODO:
- make vfremix work...
- ALU commands: complete the flags
- ALU commands: rewrite it using signed arithmetic ?
- DMA: find out how this should really work (especially add number)
- Disassembler: complete it
*/

/*DSP macros*/
#define PRF ((stv_scu[32] & 0x04000000) >> 26)
#define EPF ((stv_scu[32] & 0x02000000) >> 25)
#define T0F ((stv_scu[32] & 0x00800000) >> 23)
#define SF  ((stv_scu[32] & 0x00400000) >> 22)
#define ZF  ((stv_scu[32] & 0x00200000) >> 21)
#define CF  ((stv_scu[32] & 0x00100000) >> 20)
#define VF  ((stv_scu[32] & 0x00080000) >> 19)
#define EF  ((stv_scu[32] & 0x00040000) >> 18)
#define ESF ((stv_scu[32] & 0x00020000) >> 17)
#define EXF ((stv_scu[32] & 0x00010000) >> 16)
#define LEF ((stv_scu[32] & 0x00008000) >> 15)
#define SF_1 if(!(stv_scu[32] & 0x00400000)) stv_scu[32]^=0x00400000
#define ZF_1 if(!(stv_scu[32] & 0x00200000)) stv_scu[32]^=0x00200000
#define CF_1 if(!(stv_scu[32] & 0x00100000)) stv_scu[32]^=0x00100000
#define VF_1 if(!(stv_scu[32] & 0x00080000)) stv_scu[32]^=0x00080000
#define SF_0 if(stv_scu[32] & 0x00400000) stv_scu[32]^=0x00400000
#define ZF_0 if(stv_scu[32] & 0x00200000) stv_scu[32]^=0x00200000
#define CF_0 if(stv_scu[32] & 0x00100000) stv_scu[32]^=0x00100000
#define VF_0 if(stv_scu[32] & 0x00080000) stv_scu[32]^=0x00080000
#define T0F_1 if(!(stv_scu[32] & 0x00800000)) stv_scu[32]^=0x00800000
#define T0F_0 if(stv_scu[32] & 0x00800000) stv_scu[32]^=0x00800000

struct {
	   UINT8 pc; 							           /*Program Counter*/
	   UINT8 delay;									   /* Delay */
	   UINT8 top;							   		   /*Jump Command memory*/
	   UINT16 lop;								       /*Counter Register*/	  /*12-bits*/
	   UINT8  ct0,ct1,ct2,ct3;					       /*Index for RAM*/	  /*6-bits */
	   UINT32 md0[0x40],md1[0x40],md2[0x40],md3[0x40]; /*RAM memory*/
	   UINT8  ra;									   /*RAM selector*/
	   UINT32 rx;									   /*X-Bus register*/
	   UINT64 mul;                                     /*Multiplier register 48-bits*/
	   UINT32 ry;									   /*Y-Bus register*/
	   UINT64 alu;                                     /*ALU register       48-bits*/
	   UINT16 ph;									   /*ALU high register*/
	   UINT32 pl;									   /*ALU low register*/
	   UINT16 ach;									   /*ALU external high register*/
	   UINT32 acl;									   /*ALU external low register*/
	   UINT32 ra0,wa0;								   /*DSP DMA registers*/
	   UINT32 internal_prg[0x100];
} dsp_reg;

extern data32_t* stv_scu;

static UINT32 opcode;
static UINT8 update_mul = 0;

#define DEBUG_DSP

static	char dasm_buffer[100];
static FILE *log_file = NULL;
void dsp_dasm_opcode( UINT32 op, char *buffer );



static UINT32 dsp_get_source_mem_value( UINT32 mode )
{
	UINT32 value = 0;

	switch( mode )
	{
		case 0x0:
			value = dsp_reg.md0[ dsp_reg.ct0 ];
			break;
		case 0x1:
			value = dsp_reg.md1[ dsp_reg.ct1 ];
			break;
		case 0x2:
			value = dsp_reg.md2[ dsp_reg.ct2 ];
			break;
		case 0x3:
			value = dsp_reg.md3[ dsp_reg.ct3 ];
			break;
		case 0x4:
			value = dsp_reg.md0[ dsp_reg.ct0++ ];
			break;
		case 0x5:
			value = dsp_reg.md1[ dsp_reg.ct1++ ];
			break;
		case 0x6:
			value = dsp_reg.md2[ dsp_reg.ct2++ ];
			break;
		case 0x7:
			value = dsp_reg.md3[ dsp_reg.ct3++ ];
			break;
	}
	return value;
}

static UINT32 dsp_get_source_mem_reg_value( UINT32 mode )
{
	if ( mode < 0x8 )
	{
		return dsp_get_source_mem_value( mode );
	}
	else
	{
		switch( mode )
		{
			case 0x9:
				return (dsp_reg.alu & 0x00000000ffffffff) >> 0;
			case 0xA:
				return (dsp_reg.alu & 0x0000ffff00000000) >> 32;
		}
	}
	return 0;
}

static void dsp_set_dest_mem_reg( UINT32 mode, UINT32 value )
{
	switch( mode )
	{
		case 0x0:	/* MC0 */
			dsp_reg.md0[ dsp_reg.ct0++ ] = value;
			break;
		case 0x1:	/* MC1 */
			dsp_reg.md1[ dsp_reg.ct1++ ] = value;
			break;
		case 0x2:	/* MC2 */
			dsp_reg.md2[ dsp_reg.ct2++ ] = value;
			break;
		case 0x3:	/* MC3 */
			dsp_reg.md3[ dsp_reg.ct3++ ] = value;
			break;
		case 0x4:	/* RX */
			dsp_reg.rx = value;
			update_mul = 1;
			break;
		case 0x5:	/* PL */
			dsp_reg.pl = value;
			break;
		case 0x6:	/* RA0 */
			dsp_reg.ra0 = value;
			break;
		case 0x7:	/* WA0 */
			dsp_reg.wa0 = value;
			break;
		case 0x8:
		case 0x9:
			/* ??? */
			break;
		case 0xa:	/* LOP */
			dsp_reg.lop = value;
			break;
		case 0xb:	/* TOP */
			dsp_reg.top = value;
			break;
		case 0xc:	/* CT0 */
			dsp_reg.ct0 = value;
			break;
		case 0xd:	/* CT1 */
			dsp_reg.ct1 = value;
			break;
		case 0xe:	/* CT2 */
			dsp_reg.ct2 = value;
			break;
		case 0xf:	/* CT3 */
			dsp_reg.ct3 = value;
			break;
	}
}

static void dsp_set_dest_mem_reg_2( UINT32 mode, UINT32 value )
{
	if ( mode < 0xb )
	{
		dsp_set_dest_mem_reg( mode, value );
	}
	else
	{
		switch( mode )
		{
			case 0xc:	/* PC */
			    dsp_reg.delay = dsp_reg.pc;  /* address next after this command will be executed twice */
				dsp_reg.top = dsp_reg.pc;
				dsp_reg.pc = value;
				break;
		}
	}
}

static UINT32 dsp_compute_condition( UINT32 condition )
{
	UINT32 result = 0;

	switch( condition & 0xf )
	{
		case 0x1:	/* Z */
			result = ZF;
			break;
		case 0x2:	/* S */
			result = SF;
			break;
		case 0x3:	/* ZS */
			result = ZF | SF;
			break;
		case  0x4:	/* C */
			result = CF;
			break;
		case 0x8:	/* T0 */
			result = T0F;
			break;
	}
	if ( !(condition & 0x20) )
	{
		result = !result;
	}
	return result;
}

static void dsp_set_dest_dma_mem( UINT32 memcode, UINT32 value, UINT32 counter )
{
	if ( memcode < 4 )
	{
		dsp_set_dest_mem_reg( memcode, value );
	}
	else if ( memcode == 4 )
	{
		dsp_reg.internal_prg[ counter & 0x100 ] = value;
	}
}

static UINT32 dsp_get_mem_source_dma( UINT32 memcode, UINT32 counter )
{
	switch( memcode & 0x3 )
	{
		case 0x0:
			return dsp_reg.md0[ (dsp_reg.ct0 + counter) & 0x3f ];
		case 0x1:
			return dsp_reg.md1[ (dsp_reg.ct1 + counter) & 0x3f ];
		case 0x2:
			return dsp_reg.md2[ (dsp_reg.ct2 + counter) & 0x3f ];
		case 0x3:
			return dsp_reg.md3[ (dsp_reg.ct3 + counter) & 0x3f ];
	}
	return 0;
}

void dsp_prg_ctrl(UINT32 data)
{
	if(LEF) dsp_reg.pc = (data & 0xff);
	if(EXF) dsp_execute_program();
	if(EF && (!(stv_scu[40] & 0x0020)))
		cpu_set_irq_line_and_vector(0, 0xa, HOLD_LINE , 0x45);
}

void dsp_prg_data(UINT32 data)
{
	dsp_reg.internal_prg[dsp_reg.pc] = data;
	dsp_reg.pc++;
}

void dsp_ram_addr_ctrl(UINT32 data)
{
	dsp_reg.ra = data & 0xff;

	switch((dsp_reg.ra & 0xc0) >> 6)
	{
		case 0: dsp_reg.ct0 = (dsp_reg.ra & 0x3f); break;
		case 1: dsp_reg.ct1 = (dsp_reg.ra & 0x3f); break;
		case 2: dsp_reg.ct2 = (dsp_reg.ra & 0x3f); break;
		case 3: dsp_reg.ct3 = (dsp_reg.ra & 0x3f); break;
	}
}

void dsp_ram_addr_w(UINT32 data)
{
#ifdef DEBUG_DSP
    if ( log_file == NULL ) 
    {
        log_file = fopen( "dsp.log", "a" );
    }
    fprintf( log_file, "DSP: Writing mem %02X %04X\n", dsp_reg.ra & 0xff, data );
#endif    
	dsp_set_dest_mem_reg( (dsp_reg.ra & 0xc0) >> 6, data );
}

UINT32 dsp_ram_addr_r()
{
    UINT32 data;
    
    data = dsp_get_source_mem_value( ((dsp_reg.ra & 0xc0) >> 6) + 4 );
#ifdef DEBUG_DSP
    if ( log_file == NULL ) 
    {
        log_file = fopen( "dsp.log", "a" );
    }
    fprintf( log_file, "DSP: Reading mem at %02X %04X\n", dsp_reg.ra & 0xff, data );
#endif
	return data;
}

static void dsp_operation(void)
{
	/* ALU */
	switch( (opcode & 0x3c000000) >> 26 )
	{
		case 0x0:	/* NOP */
			break;
		case 0x1:	/* AND */
			dsp_reg.alu = dsp_reg.acl & dsp_reg.pl;
			if ( dsp_reg.alu == 0 ) {ZF_1;} else {ZF_0;}
			if ( dsp_reg.alu & 0x80000000 )
			{
				SF_1;
				dsp_reg.alu |= 0xffff00000000;	/* sign extension */
			}
			else {SF_0;}
			{CF_0;}
			break;
		case 0x2:	/* OR */
			dsp_reg.alu = dsp_reg.acl | dsp_reg.pl;
			if ( dsp_reg.alu == 0 ) {ZF_1;} else {ZF_0;}
			if ( dsp_reg.alu & 0x80000000 )
			{
				SF_1;
				dsp_reg.alu |= 0xffff00000000;	/* sign extension */
			}
			else {SF_0;}
			{CF_0;}
			break;
		case 0x3:	/* XOR */
			dsp_reg.alu = dsp_reg.acl ^ dsp_reg.pl;
			if ( dsp_reg.alu == 0 ) {ZF_1;} else {ZF_0;}
			if ( dsp_reg.alu & 0x80000000 )
			{
				SF_1;
				dsp_reg.alu |= 0xffff00000000;	/* sign extension */
			}
			else {SF_0;}
			{CF_0;}
			break;
		case 0x4:	/* ADD */
			dsp_reg.alu = dsp_reg.acl + dsp_reg.pl;
			if ( dsp_reg.alu == 0 ) {ZF_1;} else {ZF_0;}
			if ( dsp_reg.alu & 0x100000000 ) { CF_1; } else { CF_0; };
			if ( dsp_reg.alu & 0x80000000 )
			{
				SF_1;
				dsp_reg.alu |= 0xffff00000000;	/* sign extension */
			}
			else {SF_0;}
			/* TODO: V flag */
			break;
		case 0x5:	/* SUB */
			dsp_reg.alu = dsp_reg.acl - dsp_reg.pl;
			if ( dsp_reg.alu == 0 ) {ZF_1;} else {ZF_0;}
			if ( dsp_reg.alu & 0x100000000 ) { CF_1; } else { CF_0; };
			if ( dsp_reg.alu & 0x80000000 )
			{
				SF_1;
				dsp_reg.alu |= 0xffff00000000;	/* sign extension */
			}
			else {SF_0;}
			/* TODO: V flag */
			break;
		case 0x6:	/* AD2 */
			dsp_reg.alu = dsp_reg.acl + dsp_reg.pl;
			dsp_reg.alu += ((UINT64)(dsp_reg.ach + dsp_reg.ph)) << 32;
			if ( dsp_reg.alu == 0 ) {ZF_1;} else {ZF_0;}
			if ( dsp_reg.alu & 0x800000000000 ) {SF_1;} else {SF_0;}
			if ( dsp_reg.alu & 0x1000000000000 ) {CF_1; } else {CF_0;}
			/* TODO: V flag */
			break;
		case 0x7:	/* ??? */
			/* Unrecognized opcode */
			break;
		case 0x8:	/* SR */
			if ( dsp_reg.acl & 0x1 ) {CF_1;} else {CF_0;}
			dsp_reg.alu = dsp_reg.acl >> 1;
			if ( dsp_reg.alu & 0x40000000 ) dsp_reg.alu |= 0x80000000;
			if ( dsp_reg.alu == 0 ) {ZF_1;} else {ZF_0;}
			if ( dsp_reg.alu & 0x80000000 )
			{
				SF_1;
				dsp_reg.alu |= 0xffff00000000;	/* sign extension */
			}
			else {SF_0;}
			break;
		case 0x9:	/* RR */
			if ( dsp_reg.acl & 0x1 ) {CF_1;} else {CF_0;}
			dsp_reg.alu = dsp_reg.acl >> 1;
			if ( CF ) dsp_reg.alu |= 0x80000000;
			if ( dsp_reg.alu == 0 ) {ZF_1;} else {ZF_0;}
			if ( dsp_reg.alu & 0x80000000 )
			{
				SF_1;
				dsp_reg.alu |= 0xffff00000000;	/* sign extension */
			}
			else {SF_0;}
			break;
		case 0xA:	/* SL */
			if ( dsp_reg.acl & 0x80000000 ) {CF_1;} else {CF_0;}
			dsp_reg.alu = dsp_reg.acl << 1;
			if ( dsp_reg.alu == 0 ) {ZF_1;} else {ZF_0;}
			if ( dsp_reg.alu & 0x80000000 )
			{
				SF_1;
				dsp_reg.alu |= 0xffff00000000;	/* sign extension */
			}
			else {SF_0;}
			break;
		case 0xB:	/* RL */
			if ( dsp_reg.acl & 0x80000000 ) {CF_1;} else {CF_0;}
			dsp_reg.alu = dsp_reg.acl << 1;
			if ( CF ) dsp_reg.alu |= 0x1;
			if ( dsp_reg.alu == 0 ) {ZF_1;} else {ZF_0;}
			if ( dsp_reg.alu & 0x80000000 )
			{
				SF_1;
				dsp_reg.alu |= 0xffff00000000;	/* sign extension */
			}
			else {SF_0;}
			break;
		case 0xC:
		case 0xD:
		case 0xE:
			/* Unrecognized opcode */
			break;
		case 0xF:	/* RL8 */
			if ( dsp_reg.acl & 0x01000000 ) {CF_1;} else {CF_0;}
			dsp_reg.alu = (dsp_reg.acl >> 24) | (dsp_reg.acl << 8);
			if ( CF ) dsp_reg.alu |= 0x1;
			if ( dsp_reg.alu == 0 ) {ZF_1;} else {ZF_0;}
			if ( dsp_reg.alu & 0x80000000 )
			{
				SF_1;
				dsp_reg.alu |= 0xffff00000000;	/* sign extension */
			}
			else {SF_0;}
			break;
	}

	/* X-Bus */
	if ( opcode & 0x2000000 )
	{
		/* MOV [s],X */
		dsp_reg.rx = dsp_get_source_mem_value( (opcode & 0x700000) >> 20 );
		update_mul = 1;
	}
	switch( (opcode & 0x1800000) >> 23 )
	{
		case 0x0:	/* NOP */
		case 0x1:	/* NOP ? */
			break;
		case 0x2:	/* MOV MUL,P */
			dsp_reg.ph = ((dsp_reg.mul & 0x0000ffff00000000) >> 32);
			dsp_reg.pl = ((dsp_reg.mul & 0x00000000ffffffff) >> 0);
			break;
		case 0x3:	/* MOV [s],P */
			dsp_reg.pl = dsp_get_source_mem_value(  (opcode & 0x700000) >> 20 );
			if ( dsp_reg.pl & 0x80000000 ) 
			{
				dsp_reg.ph = 0xffff;
			}
			else
			{
				dsp_reg.ph = 0x0;
			}
			break;
	}

	/* Y-Bus */
	if ( opcode & 0x80000 )
	{
		/* MOV [s],Y */
		dsp_reg.ry = dsp_get_source_mem_value( (opcode & 0x1C000 ) >> 14 );
		update_mul = 1;
	}
	switch( (opcode & 0x60000) >> 17 )
	{
		case 0x0:	/* NOP */
			break;
		case 0x1:	/* CLR A */
			dsp_reg.acl = 0;
			dsp_reg.ach = 0;
			break;
		case 0x2:	/* MOV ALU,A */
			dsp_reg.ach = ((dsp_reg.alu & 0x0000ffff00000000) >> 32);
			dsp_reg.acl = ((dsp_reg.alu & 0x00000000ffffffff) >> 0);
			break;
		case 0x3:	/* MOV [s], A */
			dsp_reg.acl = dsp_get_source_mem_value( (opcode & 0x1C000 ) >> 14 );
			if ( dsp_reg.acl & 0x80000000 )
			{
				dsp_reg.ach = 0xffff;
			}
			else
			{
				dsp_reg.ach = 0x0;
			}
			break;
	}

	/* D1-Bus */
	switch( (opcode & 0x3000) >> 12 )
	{
		case 0x0:	/* NOP */
			break;
		case 0x1:	/* MOV SImm,[d] */
			dsp_set_dest_mem_reg( (opcode & 0xF00) >> 8, (INT32)(INT8)(opcode & 0xFF) );
			break;
		case 0x2:
			/* ??? */
			break;
		case 0x3:	/* MOV [s],[d] */
			dsp_set_dest_mem_reg( (opcode & 0xF00) >> 8, dsp_get_source_mem_reg_value( opcode & 0xF ) );
			break;
	}

}

static void dsp_move_immediate( void )
{
	UINT32 value;

	if ( opcode & 0x2000000 )
	{
		if ( dsp_compute_condition( (opcode & 0x3F80000 ) >> 18 ) )
		{
			value = opcode & 0x7ffff;
			if ( value & 0x40000 ) value |= 0xfff80000;
			dsp_set_dest_mem_reg_2( (opcode & 0x3C000000) >> 26, value );
		}
	}
	else
	{
		value = opcode & 0x1ffffff;
		if ( value & 0x1000000 ) value |= 0xfe000000;
		dsp_set_dest_mem_reg_2( (opcode & 0x3C000000) >> 26, value );
	}
}


static void dsp_dma( void )
{
	UINT8 hold = (opcode &  0x4000) >> 14;
	UINT32 add = (opcode & 0x38000) >> 15;
	UINT32 dir_from_D0 = (opcode & 0x1000 ) >> 12;
	UINT32 transfer_cnt = 0;
	UINT32 source = 0, dest = 0;
	UINT32 dsp_mem = (opcode & 0x700) >> 8;
	UINT32 counter = 0;
	UINT32 data;

	if ( opcode & 0x2000 )
	{
		transfer_cnt = dsp_get_source_mem_value( opcode & 0xf );
		switch ( add & 0x1 )
		{
		  case 0: add = 2; break;
		  case 1: add = 4; break;
		}
	}
	else
	{
		transfer_cnt = opcode & 0xff;
		switch( add )
		{
		  case 0: add = 0; break;  /* 0 */
		  case 1: add = 1; break;  /* 1 */
		  case 2: add = 2; break;  /* 2 */
		  case 3: add = 4; break; /* 4 */
		  case 4: add = 8; break; /* 8 */
  		  case 5: add = 16; break; /* 16 */
  		  case 6: add = 32; break; /* 32 */
  		  case 7: add = 64; break; /* 64 */
        }
	}

	if ( dir_from_D0 == 0 )
	{
		/* DMA D0,[RAM] */
		source = dsp_reg.ra0 << 2;
#ifdef DEBUG_DSP
        fprintf( log_file, "/*DSP DMA D0,[RAM%d],%d add=%d*/\n", dsp_mem, transfer_cnt, add );
#endif			
		for ( counter = 0; counter < transfer_cnt ; counter++ )
		{
		    data = cpu_readmem32bedw_dword( source );
#ifdef DEBUG_DSP
            fprintf( log_file, "%08X, %08X,\n", source, data );
#endif            		    
			dsp_set_dest_dma_mem( dsp_mem, data, counter );
			source += add;
		}

		if ( hold == 0 )
		{
			dsp_reg.ra0 += counter * add;
		}
	}
	else
	{
		/* DMA [RAM],D0 */
		dest = dsp_reg.wa0 << 2;
#ifdef DEBUG_DSP
    fprintf( log_file, "/*DSP DMA [RAM%d],D0,%d\tadd=%d,source=%08X*/\n", dsp_mem, transfer_cnt, add, source );
#endif					
		for ( counter = 0; counter < transfer_cnt; counter++ )
		{
			cpu_writemem32bedw_dword( dest, cpu_readmem32bew_word( dsp_get_mem_source_dma( dsp_mem, counter ) ) );
			dest += add;
		}

		if ( hold == 0 )
		{
			dsp_reg.wa0 += counter * add;
		}
	}
	T0F_1;
}

static void dsp_jump( void )
{
	if ( opcode & 0x3f80000 )
	{
		if ( dsp_compute_condition( (opcode & 0x3f80000) >> 19 ) )
		{
			dsp_reg.delay = dsp_reg.pc;
			dsp_reg.pc = opcode & 0xff;
		}
	}
	else
	{
		dsp_reg.delay = dsp_reg.pc;
		dsp_reg.pc = opcode & 0xff;
	}
}

static void dsp_end( void )
{
	if(opcode & 0x08000000)
	{
		/*ENDI*/
		if(!EF) stv_scu[32]^=0x00040000;
		if(EXF) stv_scu[32]^=0x00100000;
	}
	else
	{
		/*END*/
		if(EXF) stv_scu[32]^=0x00100000;
	}

};

static void dsp_loop( void )
{
	if ( opcode & 0x8000000 )
	{
		/* LPS */
		if ( dsp_reg.lop != 0 )
		{
			dsp_reg.lop--;
			dsp_reg.delay = dsp_reg.pc;
			dsp_reg.pc--;
		}
	}
	else
	{
		/* BTM */
		if ( dsp_reg.lop != 0 )
		{
			dsp_reg.lop--;
			dsp_reg.delay = dsp_reg.pc;
			dsp_reg.pc = dsp_reg.top;
		}
	}
}

static void dsp_dump_mem( FILE *f )
{
    UINT16 i;
    
	fprintf( f, "\n/*MEM 0*/\n{" );
	for ( i = 0; i < 0x40; i++ )
	{
        fprintf( f, "%08X, ", dsp_reg.md0[ i ] );
	}
	fprintf( f, "}\n/*MEM 1*/\n{" );
	for ( i = 0; i < 0x40; i++ )
	{
        fprintf( f, "%08X,", dsp_reg.md1[ i ] );
	}
	fprintf( f, "}\n/*MEM 2*/\n{" );
	for ( i = 0; i < 0x40; i++ )
	{
        fprintf( f, "%08X,", dsp_reg.md2[ i ] );
	}
	fprintf( f, "}\n/*MEM 3*/\n{" );
	for ( i = 0; i < 0x40; i++ )
	{
        fprintf( f, "%08X,", dsp_reg.md3[ i ] );
	}
	fprintf( f, "}\n" );
}

void dsp_execute_program()
{
	UINT32 cycles_run = 0;
	UINT8 cont = 1;
	UINT16 i;

#ifdef DEBUG_DSP
    if ( log_file == NULL )
    {
    	log_file = fopen("dsp.log", "a");
   	}
	for ( i = 0; i < 0x100; i++ )
	{
        dsp_dasm_opcode( dsp_reg.internal_prg[ i ], dasm_buffer );
        fprintf( log_file, "%02X\t%08X\t%s\n", i, dsp_reg.internal_prg[ i ], dasm_buffer );
	}
	dsp_dump_mem( log_file );
#endif	
	
	update_mul = 0;
	do
	{
		if ( dsp_reg.delay )
		{
			opcode = dsp_reg.internal_prg[ dsp_reg.delay ];
			dsp_reg.delay = 0;
		}
		else
		{
			opcode = dsp_reg.internal_prg[ dsp_reg.pc ];
			dsp_reg.pc++;
		}

		dsp_dasm_opcode( opcode, dasm_buffer );

		switch( (opcode & 0xc0000000) >> 30 ) 
		{
		case 0x00: /* 00 */
			dsp_operation();
			break;
		case 0x01: /* 01 */
			/* unrecognized opcode */
			break;
		case 0x02: /* 10 */
			dsp_move_immediate( );
			break;
		case 0x03: /* 11 */
			switch( (opcode & 0x30000000) >> 28 )
			{
				case 0x00:
					dsp_dma();
					break;
				case 0x01:
					dsp_jump();
					break;
				case 0x02:
					dsp_loop();
					break;
				case 0x03:
					dsp_end();
					cont = 0;
					break;
			}
			break;
		}
		if ( update_mul == 1 )
		{
			dsp_reg.mul = ((UINT64)dsp_reg.rx) * ((UINT64)dsp_reg.ry);
			update_mul = 0;
		}
		
		cycles_run++;

	} while( cont );
#ifdef DEBUG_DSP	
	dsp_dump_mem( log_file );
	fprintf( log_file, "\nRun %d cycles\n\n", cycles_run );	
	fclose( log_file );
	log_file = NULL;
#endif	
}



/***********************************************************************

    SCU DSP Disassembler
    
************************************************************************/
const char* ALU_Commands[] =
{
	"",		/* 0000 */
	"AND",	/* 0001 */
	"OR",	/* 0010 */
	"XOR",	/* 0011 */
	"ADD",	/* 0100 */
	"SUB",	/* 0101 */
	"AD2",	/* 0110 */
	"???",	/* 0111 */
	"SR",	/* 1000 */
	"RR",	/* 1001 */
	"SL",	/* 1010 */
	"RL",	/* 1011 */
	"???",	/* 1100 */
	"???",	/* 1101 */
	"???",	/* 1110 */
	"RL8",	/* 1111 */	
};

const char* X_Commands[] =
{
	"",				/* 000 */
	"",				/* 001 */	/* NOP? check instruction @ 0x0B */
	"MOV MUL,P",	/* 010 */
	"MOV %s,P",		/* 011 */
	"MOV %s,X",		/* 100 */
};

const char* Y_Commands[] =
{
	"",				/* 000 */
	"CLR A",		/* 001 */
	"MOV ALU,A",	/* 010 */
	"MOV %s,A",		/* 011 */
	"MOV %s,Y",		/* 100 */
};

const char* D1_Commands[] = 
{
	"",					/* 00 */
	"MOV %I8,%d",		/* 01 */
	"???",				/* 10 */
	"MOV %S,%d",		/* 11 */
};

const char* SourceMemory[] =
{
	"M0",			/* 000 */
	"M1",			/* 001 */
	"M2",			/* 010 */
	"M3",			/* 011 */
	"MC0",			/* 100 */
	"MC1",			/* 101 */
	"MC2",			/* 110 */
	"MC3",			/* 111 */
};

const char* SourceMemory2[] =
{
	"M0",			/* 0000 */
	"M1",			/* 0001 */
	"M2",			/* 0010 */
	"M3",			/* 0011 */
	"MC0",			/* 0100 */
	"MC1",			/* 0101 */
	"MC2",			/* 0110 */
	"MC3",			/* 0111 */
	"???",			/* 1000 */
	"ALL",			/* 1001 */
	"ALH",			/* 1010 */
	"???",			/* 1011 */
	"???",			/* 1100 */
	"???",			/* 1101 */
	"???",			/* 1110 */
	"???",			/* 1111 */
};

const char* DestMemory[] =
{
	"MC0",			/* 0000 */
	"MC1",			/* 0001 */
	"MC2",			/* 0010 */
	"MC3",			/* 0011 */
	"RX",			/* 0100 */
	"PL",			/* 0101 */
	"RA0",			/* 0110 */
	"WA0",			/* 0111 */
	"???",			/* 1000 */
	"???",			/* 1001 */
	"LOP",			/* 1010 */
	"TOP",			/* 1011 */
	"CT0",			/* 1100 */
	"CT1",			/* 1101 */
	"CT2",			/* 1110 */
	"CT3",			/* 1111 */
};

const char* DestDMAMemory[] =
{
	"M0",			/* 000 */
	"M1",			/* 001 */
	"M2",			/* 010 */
	"M3",			/* 011 */
	"PRG",			/* 100 */
	"???",			/* 101 */
	"???",			/* 110 */
	"???",			/* 111 */
};

const char* MVI_Command[] =
{
	"MVI %I,%d",	/* 0 */
	"MVI %I,%d,%f",	/* 1 */
};

const char* JMP_Command[] =
{
	"JMP %IA",
	"JMP %f,%IA",
};

const char* DMA_Command[] =
{
	"DMA%H%A D0,%M,%I",
	"DMA%H%A %s,D0,%I",
	"DMA%H%A D0,%M,%s",
	"DMA%H%A %s,D0,%s",
};


void	dsp_dasm_prefix( const char* format, char* buffer, UINT32 *data )
{
	
	for ( ; *format; format++ )
	{
		if ( *format == '%' )
		{
			switch( *++format )
			{
				case 'H':
					if ( *data )
					{
						strcpy( buffer, "H" );
					}
					else
					{
						*buffer = 0;
					}
					break;
				case 'A':
					if ( *data == 0 )
					{
						strcpy( buffer, "0" );
					}
					else if ( *data == 1 )
					{
						*buffer = 0;
					}
					else
					{
						sprintf( buffer, "%d", 1 << (*data - 1) );
					}
					break;
				case 's':
					strcpy( buffer, SourceMemory[ *data & 0x7 ] );
					break;
				case 'd':
					strcpy( buffer, DestMemory[ *data & 0xf ] );
					break;
				case 'S':
					strcpy( buffer, SourceMemory2[ *data & 0xf ] );
					break;
				case 'I':
					++format;
					if ( *format == '8' )
					{
						sprintf( buffer, "#$%x", *data );
					}
					else if ( *format == 'A' )
					{
						sprintf( buffer, "$%X", *data );
					}
					else
					{
						--format;
						sprintf( buffer, "#$%X", *data );
					}
					break;
				case 'f':
					if ( !(*data & 0x20) )
					{
						strcpy( buffer, "N" );
						buffer++;
					}
					switch( *data & 0xf )
					{
						case 0x3:
							strcpy( buffer, "ZS" );
							break;
						case 0x2:
							strcpy( buffer, "S" );
							break;
						case 0x4:
							strcpy( buffer, "C" );
							break;
						case 0x8:
							strcpy( buffer, "T0" );
							break;
						case 0x1:
							strcpy( buffer, "Z" );
							break;
						default:
							strcpy( buffer, "?" );
							break;
					}
					break;
				case 'M':
					strcpy( buffer, DestDMAMemory[ *data ] );
					break;
					
			}
			data++;
			buffer += strlen( buffer );
		}
		else
		{
			*buffer++ = *format;
		}
	}
	*buffer = 0;
}
void	dsp_dasm_operation( UINT32 op, char *buffer )
{
	char *my_buffer = buffer;
	char temp_buffer[64];
	UINT32 data[2 ];

	/* NOP */
	if ( (op & 0x3F8E3000) == 0 )
	{
		sprintf( buffer, "%-10s", "NOP" );
		return;
	}
	/* ALU */
	sprintf( my_buffer, "%-10s", ALU_Commands[ (op & 0x3c000000) >> 26] );
	my_buffer += strlen( my_buffer );

	/* X-Bus */
	data[0] = (op & 0x700000) >> 20;
	if ( op & 0x2000000 )
	{
		dsp_dasm_prefix( X_Commands[ 4 ], temp_buffer, data );
	}
	else
	{
		*temp_buffer = 0;
	}
	sprintf( my_buffer, "%-10s", temp_buffer );
	my_buffer += strlen( my_buffer );

	dsp_dasm_prefix( X_Commands[ (op & 0x1800000) >> 23 ], temp_buffer,  data );
	sprintf( my_buffer, "%-10s", temp_buffer );
	my_buffer += strlen( my_buffer );
	
	/* Y-Bus */
	data[0] = (op & 0x1C000 ) >> 14 ;
	if ( op & 0x80000 )
	{
		dsp_dasm_prefix( Y_Commands[4], temp_buffer, data );
	}
	else
	{
		*temp_buffer = 0;
	}
	sprintf( my_buffer, "%-10s", temp_buffer );
	my_buffer += strlen( my_buffer );
	
	dsp_dasm_prefix( Y_Commands[ (op & 0x60000) >> 17 ], temp_buffer,  data );
	sprintf( my_buffer, "%-10s", temp_buffer );
	my_buffer += strlen( my_buffer );

	/* D1-Bus */
	switch( (op & 0x3000) >> 12 ) 
	{
	case 0x1:
		data[0] = (op & 0xFF);
		data[1] = ((op & 0xF00) >> 8);
		break;
	case 0x3:
		data[0] = (op & 0xF);
		data[1] = ((op & 0xF00) >> 8);
		break;
	};
	dsp_dasm_prefix( D1_Commands[ (op & 0x3000) >> 12 ], temp_buffer, data );
	sprintf( my_buffer, "%-10s", temp_buffer );
}

void dsp_dasm_move_immediate( UINT32 op, char *buffer )
{
	UINT32 data[3];
	if ( (op & 0x2000000) )
	{
		data[0] = op & 0x7FFFF;
		data[1] = (op & 0x3C000000) >> 26;
		data[2] = (op & 0x3F80000 ) >> 18;
		dsp_dasm_prefix( MVI_Command[1], buffer, data ); /* TODO: bad mem*/
	}
	else
	{
		data[0] = op & 0x1FFFFFF;
		data[1] = (op & 0x3C000000) >> 26;
		dsp_dasm_prefix( MVI_Command[0], buffer, data ); /* TODO: bad mem*/
	}
}

void dsp_dasm_jump( UINT32 op, char *buffer )
{
	UINT32 data[2];
	if ( op & 0x3F80000 )
	{
		data[0] = (op & 0x3F80000) >> 19;
		data[1] = op & 0xff;
		dsp_dasm_prefix( JMP_Command[1], buffer, data );
	}
	else
	{
		data[0] = op & 0xff;
		dsp_dasm_prefix( JMP_Command[0], buffer, data );
	}
}

void dsp_dasm_loop( UINT32 op, char* buffer )
{
	if ( op & 0x8000000 )
	{
		strcpy( buffer, "LPS" );
	}
	else
	{
		strcpy( buffer, "BTM" );
	}
}

void dsp_dasm_end( UINT32 op, char* buffer )
{
	if ( op & 0x8000000 )
	{
		strcpy( buffer, "ENDI" );
	}
	else
	{
		strcpy( buffer, "END" );
	}
}

void dsp_dasm_dma( UINT32 op, char* buffer )
{
	UINT32 data[4];
	data[0] = (op &  0x4000) >> 14; /* H */
	data[1] = (op & 0x38000) >> 15; /* A */
	data[2] = (op & 0x700) >> 8; /* Mem */
	data[3] = (op & 0xff);
	dsp_dasm_prefix( DMA_Command[(op & 0x3000) >> 12], buffer, data );
}

void dsp_dasm_opcode( UINT32 op, char *buffer )
{
	switch( (op & 0xc0000000) >> 30 ) 
	{
	case 0x00: /* 00 */
		dsp_dasm_operation( op, buffer );
		break;
	case 0x01: /* 01 */
		strcpy( buffer, "???"  );
		break;
	case 0x02: /* 10 */
		dsp_dasm_move_immediate( op, buffer );
		break;
	case 0x03: /* 11 */
		switch( (op & 0x30000000) >> 28 )
		{
			case 0x00:
				dsp_dasm_dma( op, buffer );
				break;
			case 0x01:
				dsp_dasm_jump( op, buffer );
				break;
			case 0x02:
				dsp_dasm_loop( op, buffer );
				break;
			case 0x03:
				dsp_dasm_end( op, buffer );
				break;
		}
		break;
	}
};
