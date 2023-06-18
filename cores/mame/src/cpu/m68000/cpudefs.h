/*
     Definitions for the CPU-Modules
*/

#ifndef __m68000defs__
#define __m68000defs__

#include <stdlib.h>

#include <retro_inline.h>

#include "memory.h"

#ifdef __MWERKS__
#pragma require_prototypes off
#endif


typedef signed char		BYTE;
typedef unsigned char	UBYTE;
typedef unsigned short	UWORD;
typedef short			WORD;
typedef unsigned int	ULONG;  /* ULONG is a misnomer, it's in fact an int */
typedef int				LONG;
typedef unsigned int	CPTR;

/****************************************************************************/
/* Define a MC68K word. Upper bytes are always zero			    */
/****************************************************************************/
typedef union
{
 #ifdef MSB_FIRST
   struct { UBYTE h3,h2,h,l; } B;
   struct { UWORD h,l; } W;
   ULONG D;
 #else
   struct { UBYTE l,h,h2,h3; } B;
   struct { UWORD l,h; } W;
   ULONG D;
 #endif
} pair68000;


extern void Exception(int nr, CPTR oldpc);

typedef void cpuop_func(void);
extern  cpuop_func *cpufunctbl[65536];


typedef char flagtype;
#ifndef __WATCOMC__
#define READ_MEML(a,b) asm ("mov (%%esi),%%eax \n\t bswap %%eax \n\t" :"=a" (b) :"S" (a))
#define READ_MEMW(a,b) asm ("mov (%%esi),%%ax\n\t  xchg %%al,%%ah" :"=a" (b) : "S" (a))
#endif
#ifdef __WATCOMC__
LONG wat_readmeml(void *a);
#pragma aux wat_readmeml=\
       "mov eax,[esi]"\
       "bswap eax    "\
       parm [esi] \
       value [eax];
#define READ_MEML(a,b) b=wat_readmeml(a)
WORD wat_readmemw(void *a);
#pragma aux wat_readmemw=\
       "mov ax,[esi]"\
       "xchg al,ah    "\
       parm [esi] \
       value [ax];
#define READ_MEMW(a,b) b=wat_readmemw(a)
#endif

#define get_byte(a) cpu_readmem24bew((a)&0xffffff)
#define get_word(a) cpu_readmem24bew_word((a)&0xffffff)
#define get_long(a) cpu_readmem24bew_dword((a)&0xffffff)
#define put_byte(a,b) cpu_writemem24bew((a)&0xffffff,b)
#define put_word(a,b) cpu_writemem24bew_word((a)&0xffffff,b)
#define put_long(a,b) cpu_writemem24bew_dword((a)&0xffffff,b)


		/************************/
      	/* Structures for 68KEM */
		/************************/

/*	Assembler Engine Register Structure */

typedef struct
{
    ULONG d[8];             /* 0x0004 8 Data registers */
	CPTR  a[8];             /* 0x0024 8 Address registers */

    CPTR  usp;              /* 0x0044 Stack registers (They will overlap over reg_a7) */
    CPTR  isp;              /* 0x0048 */

    ULONG sr_high;     		/* 0x004C System registers */
    ULONG ccr;              /* 0x0050 CCR in Intel Format */
    ULONG x_carry;			/* 0x0054 Extended Carry */

    ULONG pc;            	/* 0x0058 Program Counter */

    ULONG IRQ_level;        /* 0x005C IRQ level you want the MC68K process (0=None)  */

    /* Backward compatible with C emulator - Only set in Debug compile */

    UWORD sr;

	int irq_state;
	int (*irq_callback)(int irqline);

#ifdef MAME_DEBUG

    /* Stuff for Debugger (it needs these fields) */

    ULONG  vbr,sfc,dfc,msp;
    double fp[8];
    ULONG  fpcr,fpsr,fpiar;

#endif

} regstruct;

#endif
