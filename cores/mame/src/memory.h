/***************************************************************************

	memory.h

	Functions which handle the CPU memory and I/O port access.

***************************************************************************/

#ifndef _MEMORY_H
#define _MEMORY_H

#include "osd_cpu.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Versions of GNU C earlier that 2.7 appear to have problems with the
 * __attribute__ definition of UNUSEDARG, so we act as if it was not a
 * GNU compiler.
 */

#ifdef __GNUC__
#if (__GNUC__ < 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ <= 7))
#define UNUSEDARG
#else
#define UNUSEDARG __attribute__((__unused__))
#endif
#else
#define UNUSEDARG
#endif



/*
 * Use __builtin_expect on GNU C 3.0 and above
 */
#ifdef __GNUC__
#if (__GNUC__ < 3)
#define UNEXPECTED(exp)	(exp)
#else
#define UNEXPECTED(exp)	 __builtin_expect((exp), 0)
#endif
#else
#define UNEXPECTED(exp)	(exp)
#endif



/***************************************************************************

	Parameters

***************************************************************************/

#ifdef MAME_DEBUG
#define CPUREADOP_SAFETY_NONE		0
#define CPUREADOP_SAFETY_PARTIAL	0
#define CPUREADOP_SAFETY_FULL		1
#else
#define CPUREADOP_SAFETY_NONE		1
#define CPUREADOP_SAFETY_PARTIAL	0
#define CPUREADOP_SAFETY_FULL		0
#endif


	
/***************************************************************************

	Basic type definitions

	These types are used for memory handlers.

***************************************************************************/

/* ----- typedefs for data and offset types ----- */
typedef UINT8			data8_t;
typedef UINT16			data16_t;
typedef UINT32			data32_t;
typedef UINT32			offs_t;

/* ----- typedefs for the various common memory/port handlers ----- */
typedef data8_t			(*read8_handler)  (UNUSEDARG offs_t offset);
typedef void			(*write8_handler) (UNUSEDARG offs_t offset, UNUSEDARG data8_t data);
typedef data16_t		(*read16_handler) (UNUSEDARG offs_t offset, UNUSEDARG data16_t mem_mask);
typedef void			(*write16_handler)(UNUSEDARG offs_t offset, UNUSEDARG data16_t data, UNUSEDARG data16_t mem_mask);
typedef data32_t		(*read32_handler) (UNUSEDARG offs_t offset, UNUSEDARG data32_t mem_mask);
typedef void			(*write32_handler)(UNUSEDARG offs_t offset, UNUSEDARG data32_t data, UNUSEDARG data32_t mem_mask);
typedef offs_t			(*opbase_handler) (UNUSEDARG offs_t address);

/* ----- typedefs for the various common memory handlers ----- */
typedef read8_handler	mem_read_handler;
typedef write8_handler	mem_write_handler;
typedef read16_handler	mem_read16_handler;
typedef write16_handler	mem_write16_handler;
typedef read32_handler	mem_read32_handler;
typedef write32_handler	mem_write32_handler;

/* ----- typedefs for the various common port handlers ----- */
typedef read8_handler	port_read_handler;
typedef write8_handler	port_write_handler;
typedef read16_handler	port_read16_handler;
typedef write16_handler	port_write16_handler;
typedef read32_handler	port_read32_handler;
typedef write32_handler	port_write32_handler;

/* ----- typedefs for externally allocated memory ----- */
struct ExtMemory
{
	offs_t 			start, end;
	UINT8			region;
    UINT8 *			data;
};



/***************************************************************************

	Basic macros

***************************************************************************/

/* ----- macros for declaring the various common memory/port handlers ----- */
#define READ_HANDLER(name) 		data8_t  name(UNUSEDARG offs_t offset)
#define WRITE_HANDLER(name) 	void     name(UNUSEDARG offs_t offset, UNUSEDARG data8_t data)
#define READ16_HANDLER(name)	data16_t name(UNUSEDARG offs_t offset, UNUSEDARG data16_t mem_mask)
#define WRITE16_HANDLER(name)	void     name(UNUSEDARG offs_t offset, UNUSEDARG data16_t data, UNUSEDARG data16_t mem_mask)
#define READ32_HANDLER(name)	data32_t name(UNUSEDARG offs_t offset, UNUSEDARG data32_t mem_mask)
#define WRITE32_HANDLER(name)	void     name(UNUSEDARG offs_t offset, UNUSEDARG data32_t data, UNUSEDARG data32_t mem_mask)
#define OPBASE_HANDLER(name)	offs_t   name(UNUSEDARG offs_t address)

/* ----- macros for accessing bytes and words within larger chunks ----- */
#ifdef MSB_FIRST
	#define BYTE_XOR_BE(a)  	(a)
	#define BYTE_XOR_LE(a)  	((a) ^ 1)				/* read/write a byte to a 16-bit space */
	#define BYTE4_XOR_BE(a) 	(a)
	#define BYTE4_XOR_LE(a) 	((a) ^ 3)				/* read/write a byte to a 32-bit space */
	#define WORD_XOR_BE(a)  	(a)
	#define WORD_XOR_LE(a)  	((a) ^ 2)				/* read/write a word to a 32-bit space */
#else
	#define BYTE_XOR_BE(a)  	((a) ^ 1)				/* read/write a byte to a 16-bit space */
	#define BYTE_XOR_LE(a)  	(a)
	#define BYTE4_XOR_BE(a) 	((a) ^ 3)				/* read/write a byte to a 32-bit space */
	#define BYTE4_XOR_LE(a) 	(a)
	#define WORD_XOR_BE(a)  	((a) ^ 2)				/* read/write a word to a 32-bit space */
	#define WORD_XOR_LE(a)  	(a)
#endif



/***************************************************************************

	Memory/port array constants

	These apply to values in the array of read/write handlers that is
	declared within each driver.

***************************************************************************/

/* ----- memory/port width constants ----- */
#define MEMPORT_WIDTH_MASK		0x00000003				/* mask to get at the width bits */
#define MEMPORT_WIDTH_8			0x00000001				/* this memory/port array is for an 8-bit databus */
#define MEMPORT_WIDTH_16 		0x00000002				/* this memory/port array is for a 16-bit databus */
#define MEMPORT_WIDTH_32 		0x00000003				/* this memory/port array is for a 32-bit databus */

/* ----- memory/port type constants ----- */
#define MEMPORT_TYPE_MASK		0x30000000				/* mask to get at the type bits */
#define MEMPORT_TYPE_MEM 		0x10000000				/* this memory/port array is for memory */
#define MEMPORT_TYPE_IO			0x20000000				/* this memory/port array is for ports */

/* ----- memory/port direction constants ----- */
#define MEMPORT_DIRECTION_MASK	0xc0000000				/* mask to get at the direction bits */
#define MEMPORT_DIRECTION_READ	0x40000000				/* this memory/port array is for reads */
#define MEMPORT_DIRECTION_WRITE	0x80000000				/* this memory/port array is for writes */

/* ----- memory/port address bits constants ----- */
#define MEMPORT_ABITS_MASK		0x08000000				/* set this bit to indicate the entry has address bits */
#define MEMPORT_ABITS_VAL_MASK	0x000000ff				/* number of address bits */

/* ----- memory/port struct marker constants ----- */
#define MEMPORT_MARKER			((offs_t)~0)			/* used in the end field to indicate end of array */

/* ----- static memory/port handler constants ----- */
#define STATIC_INVALID			0						/* invalid - should never be used */
#define STATIC_BANK1			1						/* banked memory #1 */
#define STATIC_BANK2			2						/* banked memory #2 */
#define STATIC_BANK3			3						/* banked memory #3 */
#define STATIC_BANK4			4						/* banked memory #4 */
#define STATIC_BANK5			5						/* banked memory #5 */
#define STATIC_BANK6			6						/* banked memory #6 */
#define STATIC_BANK7			7						/* banked memory #7 */
#define STATIC_BANK8			8						/* banked memory #8 */
#define STATIC_BANK9			9						/* banked memory #9 */
#define STATIC_BANK10			10						/* banked memory #10 */
#define STATIC_BANK11			11						/* banked memory #11 */
#define STATIC_BANK12			12						/* banked memory #12 */
#define STATIC_BANK13			13						/* banked memory #13 */
#define STATIC_BANK14			14						/* banked memory #14 */
#define STATIC_BANK15			15						/* banked memory #15 */
#define STATIC_BANK16			16						/* banked memory #16 */
#define STATIC_BANK17			17						/* banked memory #17 */
#define STATIC_BANK18			18						/* banked memory #18 */
#define STATIC_BANK19			19						/* banked memory #19 */
#define STATIC_BANK20			20						/* banked memory #20 */
#define STATIC_BANK21			21						/* banked memory #21 */
#define STATIC_BANK22			22						/* banked memory #22 */
#define STATIC_BANK23			23						/* banked memory #23 */
#define STATIC_BANK24			24						/* banked memory #24 */
#define STATIC_RAM				25						/* RAM - standard reads/writes */
#define STATIC_ROM				26						/* ROM - just like RAM, but writes to the bit-bucket */
#define STATIC_RAMROM			27						/* RAMROM - use for access in encrypted 8-bit systems */
#define STATIC_NOP				28						/* NOP - reads are 0, writes to the bit-bucket */
#define STATIC_UNUSED1			29						/* unused - reserved for future use */
#define STATIC_UNUSED2			30						/* unused - reserved for future use */
#define STATIC_UNMAP			31						/* unmapped - all unmapped memory goes here */
#define STATIC_COUNT			32						/* total number of static handlers */

/* ----- banking constants ----- */
#define MAX_BANKS				24						/* maximum number of banks */
#define STATIC_BANKMAX			(STATIC_RAM - 1)		/* handler constant of last bank */



/***************************************************************************

	Constants for static entries in memory read/write arrays

	The first 32 entries in the memory lookup table are reserved for
	"static" handlers. These are internal handlers for RAM, ROM, banks,
	and unmapped memory areas. The following definitions are the
	properly-casted versions of the STATIC_ constants above.

***************************************************************************/

/* 8-bit reads */
#define MRA_BANK1				((mem_read_handler)STATIC_BANK1)
#define MRA_BANK2				((mem_read_handler)STATIC_BANK2)
#define MRA_BANK3				((mem_read_handler)STATIC_BANK3)
#define MRA_BANK4				((mem_read_handler)STATIC_BANK4)
#define MRA_BANK5				((mem_read_handler)STATIC_BANK5)
#define MRA_BANK6				((mem_read_handler)STATIC_BANK6)
#define MRA_BANK7				((mem_read_handler)STATIC_BANK7)
#define MRA_BANK8				((mem_read_handler)STATIC_BANK8)
#define MRA_BANK9				((mem_read_handler)STATIC_BANK9)
#define MRA_BANK10				((mem_read_handler)STATIC_BANK10)
#define MRA_BANK11				((mem_read_handler)STATIC_BANK11)
#define MRA_BANK12				((mem_read_handler)STATIC_BANK12)
#define MRA_BANK13				((mem_read_handler)STATIC_BANK13)
#define MRA_BANK14				((mem_read_handler)STATIC_BANK14)
#define MRA_BANK15				((mem_read_handler)STATIC_BANK15)
#define MRA_BANK16				((mem_read_handler)STATIC_BANK16)
#define MRA_BANK17				((mem_read_handler)STATIC_BANK17)
#define MRA_BANK18				((mem_read_handler)STATIC_BANK18)
#define MRA_BANK19				((mem_read_handler)STATIC_BANK19)
#define MRA_BANK20				((mem_read_handler)STATIC_BANK20)
#define MRA_BANK21				((mem_read_handler)STATIC_BANK21)
#define MRA_BANK22				((mem_read_handler)STATIC_BANK22)
#define MRA_BANK23				((mem_read_handler)STATIC_BANK23)
#define MRA_BANK24				((mem_read_handler)STATIC_BANK24)
#define MRA_NOP					((mem_read_handler)STATIC_NOP)
#define MRA_RAM					((mem_read_handler)STATIC_RAM)
#define MRA_ROM					((mem_read_handler)STATIC_ROM)
#define MRA_RAMROM				((mem_read_handler)STATIC_RAMROM)

/* 8-bit writes */
#define MWA_BANK1				((mem_write_handler)STATIC_BANK1)
#define MWA_BANK2				((mem_write_handler)STATIC_BANK2)
#define MWA_BANK3				((mem_write_handler)STATIC_BANK3)
#define MWA_BANK4				((mem_write_handler)STATIC_BANK4)
#define MWA_BANK5				((mem_write_handler)STATIC_BANK5)
#define MWA_BANK6				((mem_write_handler)STATIC_BANK6)
#define MWA_BANK7				((mem_write_handler)STATIC_BANK7)
#define MWA_BANK8				((mem_write_handler)STATIC_BANK8)
#define MWA_BANK9				((mem_write_handler)STATIC_BANK9)
#define MWA_BANK10				((mem_write_handler)STATIC_BANK10)
#define MWA_BANK11				((mem_write_handler)STATIC_BANK11)
#define MWA_BANK12				((mem_write_handler)STATIC_BANK12)
#define MWA_BANK13				((mem_write_handler)STATIC_BANK13)
#define MWA_BANK14				((mem_write_handler)STATIC_BANK14)
#define MWA_BANK15				((mem_write_handler)STATIC_BANK15)
#define MWA_BANK16				((mem_write_handler)STATIC_BANK16)
#define MWA_BANK17				((mem_write_handler)STATIC_BANK17)
#define MWA_BANK18				((mem_write_handler)STATIC_BANK18)
#define MWA_BANK19				((mem_write_handler)STATIC_BANK19)
#define MWA_BANK20				((mem_write_handler)STATIC_BANK20)
#define MWA_BANK21				((mem_write_handler)STATIC_BANK21)
#define MWA_BANK22				((mem_write_handler)STATIC_BANK22)
#define MWA_BANK23				((mem_write_handler)STATIC_BANK23)
#define MWA_BANK24				((mem_write_handler)STATIC_BANK24)
#define MWA_NOP					((mem_write_handler)STATIC_NOP)
#define MWA_RAM					((mem_write_handler)STATIC_RAM)
#define MWA_ROM					((mem_write_handler)STATIC_ROM)
#define MWA_RAMROM				((mem_write_handler)STATIC_RAMROM)

/* 16-bit reads */
#define MRA16_BANK1				((mem_read16_handler)STATIC_BANK1)
#define MRA16_BANK2				((mem_read16_handler)STATIC_BANK2)
#define MRA16_BANK3				((mem_read16_handler)STATIC_BANK3)
#define MRA16_BANK4				((mem_read16_handler)STATIC_BANK4)
#define MRA16_BANK5				((mem_read16_handler)STATIC_BANK5)
#define MRA16_BANK6				((mem_read16_handler)STATIC_BANK6)
#define MRA16_BANK7				((mem_read16_handler)STATIC_BANK7)
#define MRA16_BANK8				((mem_read16_handler)STATIC_BANK8)
#define MRA16_BANK9				((mem_read16_handler)STATIC_BANK9)
#define MRA16_BANK10			((mem_read16_handler)STATIC_BANK10)
#define MRA16_BANK11			((mem_read16_handler)STATIC_BANK11)
#define MRA16_BANK12			((mem_read16_handler)STATIC_BANK12)
#define MRA16_BANK13			((mem_read16_handler)STATIC_BANK13)
#define MRA16_BANK14			((mem_read16_handler)STATIC_BANK14)
#define MRA16_BANK15			((mem_read16_handler)STATIC_BANK15)
#define MRA16_BANK16			((mem_read16_handler)STATIC_BANK16)
#define MRA16_BANK17			((mem_read16_handler)STATIC_BANK17)
#define MRA16_BANK18			((mem_read16_handler)STATIC_BANK18)
#define MRA16_BANK19			((mem_read16_handler)STATIC_BANK19)
#define MRA16_BANK20			((mem_read16_handler)STATIC_BANK20)
#define MRA16_BANK21			((mem_read16_handler)STATIC_BANK21)
#define MRA16_BANK22			((mem_read16_handler)STATIC_BANK22)
#define MRA16_BANK23			((mem_read16_handler)STATIC_BANK23)
#define MRA16_BANK24			((mem_read16_handler)STATIC_BANK24)
#define MRA16_NOP				((mem_read16_handler)STATIC_NOP)
#define MRA16_RAM				((mem_read16_handler)STATIC_RAM)
#define MRA16_ROM				((mem_read16_handler)STATIC_ROM)

/* 16-bit writes */
#define MWA16_BANK1				((mem_write16_handler)STATIC_BANK1)
#define MWA16_BANK2				((mem_write16_handler)STATIC_BANK2)
#define MWA16_BANK3				((mem_write16_handler)STATIC_BANK3)
#define MWA16_BANK4				((mem_write16_handler)STATIC_BANK4)
#define MWA16_BANK5				((mem_write16_handler)STATIC_BANK5)
#define MWA16_BANK6				((mem_write16_handler)STATIC_BANK6)
#define MWA16_BANK7				((mem_write16_handler)STATIC_BANK7)
#define MWA16_BANK8				((mem_write16_handler)STATIC_BANK8)
#define MWA16_BANK9				((mem_write16_handler)STATIC_BANK9)
#define MWA16_BANK10			((mem_write16_handler)STATIC_BANK10)
#define MWA16_BANK11			((mem_write16_handler)STATIC_BANK11)
#define MWA16_BANK12			((mem_write16_handler)STATIC_BANK12)
#define MWA16_BANK13			((mem_write16_handler)STATIC_BANK13)
#define MWA16_BANK14			((mem_write16_handler)STATIC_BANK14)
#define MWA16_BANK15			((mem_write16_handler)STATIC_BANK15)
#define MWA16_BANK16			((mem_write16_handler)STATIC_BANK16)
#define MWA16_BANK17			((mem_write16_handler)STATIC_BANK17)
#define MWA16_BANK18			((mem_write16_handler)STATIC_BANK18)
#define MWA16_BANK19			((mem_write16_handler)STATIC_BANK19)
#define MWA16_BANK20			((mem_write16_handler)STATIC_BANK20)
#define MWA16_BANK21			((mem_write16_handler)STATIC_BANK21)
#define MWA16_BANK22			((mem_write16_handler)STATIC_BANK22)
#define MWA16_BANK23			((mem_write16_handler)STATIC_BANK23)
#define MWA16_BANK24			((mem_write16_handler)STATIC_BANK24)
#define MWA16_NOP				((mem_write16_handler)STATIC_NOP)
#define MWA16_RAM				((mem_write16_handler)STATIC_RAM)
#define MWA16_ROM				((mem_write16_handler)STATIC_ROM)

/* 32-bit reads */
#define MRA32_BANK1				((mem_read32_handler)STATIC_BANK1)
#define MRA32_BANK2				((mem_read32_handler)STATIC_BANK2)
#define MRA32_BANK3				((mem_read32_handler)STATIC_BANK3)
#define MRA32_BANK4				((mem_read32_handler)STATIC_BANK4)
#define MRA32_BANK5				((mem_read32_handler)STATIC_BANK5)
#define MRA32_BANK6				((mem_read32_handler)STATIC_BANK6)
#define MRA32_BANK7				((mem_read32_handler)STATIC_BANK7)
#define MRA32_BANK8				((mem_read32_handler)STATIC_BANK8)
#define MRA32_BANK9				((mem_read32_handler)STATIC_BANK9)
#define MRA32_BANK10			((mem_read32_handler)STATIC_BANK10)
#define MRA32_BANK11			((mem_read32_handler)STATIC_BANK11)
#define MRA32_BANK12			((mem_read32_handler)STATIC_BANK12)
#define MRA32_BANK13			((mem_read32_handler)STATIC_BANK13)
#define MRA32_BANK14			((mem_read32_handler)STATIC_BANK14)
#define MRA32_BANK15			((mem_read32_handler)STATIC_BANK15)
#define MRA32_BANK16			((mem_read32_handler)STATIC_BANK16)
#define MRA32_BANK17			((mem_read32_handler)STATIC_BANK17)
#define MRA32_BANK18			((mem_read32_handler)STATIC_BANK18)
#define MRA32_BANK19			((mem_read32_handler)STATIC_BANK19)
#define MRA32_BANK20			((mem_read32_handler)STATIC_BANK20)
#define MRA32_BANK21			((mem_read32_handler)STATIC_BANK21)
#define MRA32_BANK22			((mem_read32_handler)STATIC_BANK22)
#define MRA32_BANK23			((mem_read32_handler)STATIC_BANK23)
#define MRA32_BANK24			((mem_read32_handler)STATIC_BANK24)
#define MRA32_NOP				((mem_read32_handler)STATIC_NOP)
#define MRA32_RAM				((mem_read32_handler)STATIC_RAM)
#define MRA32_ROM				((mem_read32_handler)STATIC_ROM)

/* 32-bit writes */
#define MWA32_BANK1				((mem_write32_handler)STATIC_BANK1)
#define MWA32_BANK2				((mem_write32_handler)STATIC_BANK2)
#define MWA32_BANK3				((mem_write32_handler)STATIC_BANK3)
#define MWA32_BANK4				((mem_write32_handler)STATIC_BANK4)
#define MWA32_BANK5				((mem_write32_handler)STATIC_BANK5)
#define MWA32_BANK6				((mem_write32_handler)STATIC_BANK6)
#define MWA32_BANK7				((mem_write32_handler)STATIC_BANK7)
#define MWA32_BANK8				((mem_write32_handler)STATIC_BANK8)
#define MWA32_BANK9				((mem_write32_handler)STATIC_BANK9)
#define MWA32_BANK10			((mem_write32_handler)STATIC_BANK10)
#define MWA32_BANK11			((mem_write32_handler)STATIC_BANK11)
#define MWA32_BANK12			((mem_write32_handler)STATIC_BANK12)
#define MWA32_BANK13			((mem_write32_handler)STATIC_BANK13)
#define MWA32_BANK14			((mem_write32_handler)STATIC_BANK14)
#define MWA32_BANK15			((mem_write32_handler)STATIC_BANK15)
#define MWA32_BANK16			((mem_write32_handler)STATIC_BANK16)
#define MWA32_BANK17			((mem_write32_handler)STATIC_BANK17)
#define MWA32_BANK18			((mem_write32_handler)STATIC_BANK18)
#define MWA32_BANK19			((mem_write32_handler)STATIC_BANK19)
#define MWA32_BANK20			((mem_write32_handler)STATIC_BANK20)
#define MWA32_BANK21			((mem_write32_handler)STATIC_BANK21)
#define MWA32_BANK22			((mem_write32_handler)STATIC_BANK22)
#define MWA32_BANK23			((mem_write32_handler)STATIC_BANK23)
#define MWA32_BANK24			((mem_write32_handler)STATIC_BANK24)
#define MWA32_NOP				((mem_write32_handler)STATIC_NOP)
#define MWA32_RAM				((mem_write32_handler)STATIC_RAM)
#define MWA32_ROM				((mem_write32_handler)STATIC_ROM)



/***************************************************************************

	Constants for static entries in port read/write arrays

***************************************************************************/

/* 8-bit port reads */
#define IORP_NOP				((port_read_handler)STATIC_NOP)

/* 8-bit port writes */
#define IOWP_NOP				((port_write_handler)STATIC_NOP)

/* 16-bit port reads */
#define IORP16_NOP				((port_read16_handler)STATIC_NOP)

/* 16-bit port writes */
#define IOWP16_NOP				((port_write16_handler)STATIC_NOP)

/* 32-bit port reads */
#define IORP32_NOP				((port_read32_handler)STATIC_NOP)

/* 32-bit port writes */
#define IOWP32_NOP				((port_write32_handler)STATIC_NOP)



/***************************************************************************

	Memory/port array type definitions

	Note that the memory hooks are not passed the actual memory address
	where the operation takes place, but the offset from the beginning
	of the block they are assigned to. This makes handling of mirror
	addresses easier, and makes the handlers a bit more "object oriented".
	If you handler needs to read/write the main memory area, provide a
	"base" pointer: it will be initialized by the main engine to point to
	the beginning of the memory block assigned to the handler. You may
	also provided a pointer to "size": it will be set to the length of
	the memory area processed by the handler.

***************************************************************************/

/* ----- structs for memory read arrays ----- */
struct Memory_ReadAddress
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	mem_read_handler 	handler;		/* handler callback */
};

struct Memory_ReadAddress16
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	mem_read16_handler 	handler;		/* handler callback */
};

struct Memory_ReadAddress32
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	mem_read32_handler	handler;		/* handler callback */
};

/* ----- structs for memory write arrays ----- */
struct Memory_WriteAddress
{
    offs_t				start, end;		/* start, end addresses, inclusive */
	mem_write_handler	handler;		/* handler callback */
	data8_t **			base;			/* receives pointer to memory (optional) */
    size_t *			size;			/* receives size of memory in bytes (optional) */
};

struct Memory_WriteAddress16
{
    offs_t				start, end;		/* start, end addresses, inclusive */
	mem_write16_handler handler;		/* handler callback */
	data16_t **			base;			/* receives pointer to memory (optional) */
    size_t *			size;			/* receives size of memory in bytes (optional) */
};

struct Memory_WriteAddress32
{
    offs_t				start, end;		/* start, end addresses, inclusive */
	mem_write32_handler handler;		/* handler callback */
	data32_t **			base;			/* receives pointer to memory (optional) */
	size_t *			size;			/* receives size of memory in bytes (optional) */
};

/* ----- structs for port read arrays ----- */
struct IO_ReadPort
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	port_read_handler 	handler;		/* handler callback */
};

struct IO_ReadPort16
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	port_read16_handler	handler;		/* handler callback */
};

struct IO_ReadPort32
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	port_read32_handler	handler;		/* handler callback */
};

/* ----- structs for port write arrays ----- */
struct IO_WritePort
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	port_write_handler	handler;		/* handler callback */
};

struct IO_WritePort16
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	port_write16_handler handler;		/* handler callback */
};

struct IO_WritePort32
{
	offs_t				start, end;		/* start, end addresses, inclusive */
	port_write32_handler handler;		/* handler callback */
};



/***************************************************************************

	Memory/port array macros

***************************************************************************/

/* ----- macros for identifying memory/port struct markers ----- */
#define IS_MEMPORT_MARKER(ma)		((ma)->start == MEMPORT_MARKER && (ma)->end < MEMPORT_MARKER)
#define IS_MEMPORT_END(ma)			((ma)->start == MEMPORT_MARKER && (ma)->end == 0)

/* ----- macros for defining the start/stop points ----- */
#define MEMPORT_ARRAY_START(t,n,f)	const struct t n[] = { { MEMPORT_MARKER, (f) },
#define MEMPORT_ARRAY_END			{ MEMPORT_MARKER, 0 } };

/* ----- macros for setting the number of address bits ----- */
#define MEMPORT_SET_BITS(b)			{ MEMPORT_MARKER, MEMPORT_ABITS_MASK | (b) },

/* ----- macros for declaring the start of a memory struct array ----- */
#define MEMORY_READ_START(name)		MEMPORT_ARRAY_START(Memory_ReadAddress,    name, MEMPORT_DIRECTION_READ  | MEMPORT_TYPE_MEM | MEMPORT_WIDTH_8)
#define MEMORY_WRITE_START(name)	MEMPORT_ARRAY_START(Memory_WriteAddress,   name, MEMPORT_DIRECTION_WRITE | MEMPORT_TYPE_MEM | MEMPORT_WIDTH_8)
#define MEMORY_READ16_START(name)	MEMPORT_ARRAY_START(Memory_ReadAddress16,  name, MEMPORT_DIRECTION_READ  | MEMPORT_TYPE_MEM | MEMPORT_WIDTH_16)
#define MEMORY_WRITE16_START(name)	MEMPORT_ARRAY_START(Memory_WriteAddress16, name, MEMPORT_DIRECTION_WRITE | MEMPORT_TYPE_MEM | MEMPORT_WIDTH_16)
#define MEMORY_READ32_START(name)	MEMPORT_ARRAY_START(Memory_ReadAddress32,  name, MEMPORT_DIRECTION_READ  | MEMPORT_TYPE_MEM | MEMPORT_WIDTH_32)
#define MEMORY_WRITE32_START(name)	MEMPORT_ARRAY_START(Memory_WriteAddress32, name, MEMPORT_DIRECTION_WRITE | MEMPORT_TYPE_MEM | MEMPORT_WIDTH_32)

#define MEMORY_ADDRESS_BITS(bits)	MEMPORT_SET_BITS(bits)
#define MEMORY_END					MEMPORT_ARRAY_END

/* ----- macros for declaring the start of a port struct array ----- */
#define PORT_READ_START(name)		MEMPORT_ARRAY_START(IO_ReadPort,    name, MEMPORT_DIRECTION_READ  | MEMPORT_TYPE_IO | MEMPORT_WIDTH_8)
#define PORT_WRITE_START(name)		MEMPORT_ARRAY_START(IO_WritePort,   name, MEMPORT_DIRECTION_WRITE | MEMPORT_TYPE_IO | MEMPORT_WIDTH_8)
#define PORT_READ16_START(name)		MEMPORT_ARRAY_START(IO_ReadPort16,  name, MEMPORT_DIRECTION_READ  | MEMPORT_TYPE_IO | MEMPORT_WIDTH_16)
#define PORT_WRITE16_START(name)	MEMPORT_ARRAY_START(IO_WritePort16, name, MEMPORT_DIRECTION_WRITE | MEMPORT_TYPE_IO | MEMPORT_WIDTH_16)
#define PORT_READ32_START(name)		MEMPORT_ARRAY_START(IO_ReadPort32,  name, MEMPORT_DIRECTION_READ  | MEMPORT_TYPE_IO | MEMPORT_WIDTH_32)
#define PORT_WRITE32_START(name)	MEMPORT_ARRAY_START(IO_WritePort32, name, MEMPORT_DIRECTION_WRITE | MEMPORT_TYPE_IO | MEMPORT_WIDTH_32)

#define PORT_ADDRESS_BITS(bits)		MEMPORT_SET_BITS(bits)
#define PORT_END					MEMPORT_ARRAY_END



/***************************************************************************

	Memory/port lookup constants

	These apply to values in the internal lookup table.

***************************************************************************/

/* ----- memory/port lookup table definitions ----- */
#define SUBTABLE_COUNT			64						/* number of slots reserved for subtables */
#define SUBTABLE_MASK			(SUBTABLE_COUNT-1)		/* mask to get at the subtable index */
#define SUBTABLE_BASE			(256-SUBTABLE_COUNT)	/* first index of a subtable */
#define ENTRY_COUNT				(SUBTABLE_BASE)			/* number of legitimate (non-subtable) entries */
#define SUBTABLE_ALLOC			8						/* number of subtables to allocate at a time */

/* ----- bit counts ----- */
#define LEVEL1_BITS_PREF		12						/* preferred number of bits in the 1st level lookup */
#define LEVEL1_BITS_BIAS		4						/* number of bits used to bias the L1 bits computation */
#define SPARSE_THRESH			20						/* number of address bits above which we use sparse memory */

/* ----- external memory constants ----- */
#define MAX_EXT_MEMORY			64						/* maximum external memory areas we can allocate */



/***************************************************************************

	Memory/port lookup macros

	These are used for accessing the internal lookup table.

***************************************************************************/

/* ----- macros for determining the number of bits to use ----- */
#define LEVEL1_BITS(x)			(((x) < (2*LEVEL1_BITS_PREF - LEVEL1_BITS_BIAS)) ? LEVEL1_BITS_PREF : ((x) + LEVEL1_BITS_BIAS) / 2)
#define LEVEL2_BITS(x)			((x) - LEVEL1_BITS(x))
#define LEVEL1_MASK(x)			((1 << LEVEL1_BITS(x)) - 1)
#define LEVEL2_MASK(x)			((1 << LEVEL2_BITS(x)) - 1)

/* ----- table lookup helpers ----- */
#define LEVEL1_INDEX(a,b,m)		((a) >> (LEVEL2_BITS((b)-(m)) + (m)))
#define LEVEL2_INDEX(e,a,b,m)	((1 << LEVEL1_BITS((b)-(m))) + (((e) & SUBTABLE_MASK) << LEVEL2_BITS((b)-(m))) + (((a) >> (m)) & LEVEL2_MASK((b)-(m))))

/* ----- sparse memory space detection ----- */
#define IS_SPARSE(a)			((a) > SPARSE_THRESH)



/***************************************************************************

	Macros to help declare handlers for core readmem/writemem routines

***************************************************************************/

/* ----- for declaring 8-bit handlers ----- */
#define DECLARE_HANDLERS_8BIT(type, abits) \
data8_t  cpu_read##type##abits             (offs_t offset);					\
void     cpu_write##type##abits            (offs_t offset, data8_t data);

/* ----- for declaring 16-bit bigendian handlers ----- */
#define DECLARE_HANDLERS_16BIT_BE(type, abits) \
data8_t  cpu_read##type##abits##bew        (offs_t offset);					\
data16_t cpu_read##type##abits##bew_word   (offs_t offset);					\
void     cpu_write##type##abits##bew       (offs_t offset, data8_t data);	\
void     cpu_write##type##abits##bew_word  (offs_t offset, data16_t data);

/* ----- for declaring 16-bit littleendian handlers ----- */
#define DECLARE_HANDLERS_16BIT_LE(type, abits) \
data8_t  cpu_read##type##abits##lew        (offs_t offset);					\
data16_t cpu_read##type##abits##lew_word   (offs_t offset);					\
void     cpu_write##type##abits##lew       (offs_t offset, data8_t data);	\
void     cpu_write##type##abits##lew_word  (offs_t offset, data16_t data);

/* ----- for declaring 32-bit bigendian handlers ----- */
#define DECLARE_HANDLERS_32BIT_BE(type, abits) \
data8_t  cpu_read##type##abits##bedw       (offs_t offset);					\
data16_t cpu_read##type##abits##bedw_word  (offs_t offset);					\
data32_t cpu_read##type##abits##bedw_dword (offs_t offset);					\
void     cpu_write##type##abits##bedw      (offs_t offset, data8_t data);	\
void     cpu_write##type##abits##bedw_word (offs_t offset, data16_t data);	\
void     cpu_write##type##abits##bedw_dword(offs_t offset, data32_t data);

/* ----- for declaring 32-bit littleendian handlers ----- */
#define DECLARE_HANDLERS_32BIT_LE(type, abits) \
data8_t  cpu_read##type##abits##ledw       (offs_t offset);					\
data16_t cpu_read##type##abits##ledw_word  (offs_t offset);					\
data32_t cpu_read##type##abits##ledw_dword (offs_t offset);					\
void     cpu_write##type##abits##ledw      (offs_t offset, data8_t data);	\
void     cpu_write##type##abits##ledw_word (offs_t offset, data16_t data);	\
void     cpu_write##type##abits##ledw_dword(offs_t offset, data32_t data);

/* ----- for declaring memory handlers ----- */
#define DECLARE_MEM_HANDLERS_8BIT(abits) \
DECLARE_HANDLERS_8BIT(mem, abits) \
void     cpu_setopbase##abits              (offs_t pc);

#define DECLARE_MEM_HANDLERS_16BIT_BE(abits) \
DECLARE_HANDLERS_16BIT_BE(mem, abits) \
void     cpu_setopbase##abits##bew         (offs_t pc);

#define DECLARE_MEM_HANDLERS_16BIT_LE(abits) \
DECLARE_HANDLERS_16BIT_LE(mem, abits) \
void     cpu_setopbase##abits##lew         (offs_t pc);

#define DECLARE_MEM_HANDLERS_32BIT_BE(abits) \
DECLARE_HANDLERS_32BIT_BE(mem, abits) \
void     cpu_setopbase##abits##bedw        (offs_t pc);

#define DECLARE_MEM_HANDLERS_32BIT_LE(abits) \
DECLARE_HANDLERS_32BIT_LE(mem, abits) \
void     cpu_setopbase##abits##ledw        (offs_t pc);

/* ----- for declaring port handlers ----- */
#define DECLARE_PORT_HANDLERS_8BIT(abits) \
DECLARE_HANDLERS_8BIT(port, abits)

#define DECLARE_PORT_HANDLERS_16BIT_BE(abits) \
DECLARE_HANDLERS_16BIT_BE(port, abits)

#define DECLARE_PORT_HANDLERS_16BIT_LE(abits) \
DECLARE_HANDLERS_16BIT_LE(port, abits)

#define DECLARE_PORT_HANDLERS_32BIT_BE(abits) \
DECLARE_HANDLERS_32BIT_BE(port, abits)

#define DECLARE_PORT_HANDLERS_32BIT_LE(abits) \
DECLARE_HANDLERS_32BIT_LE(port, abits)



/***************************************************************************

	Function prototypes for core readmem/writemem routines

***************************************************************************/

/* ----- declare 8-bit handlers ----- */
DECLARE_MEM_HANDLERS_8BIT(16)
DECLARE_MEM_HANDLERS_8BIT(17)
DECLARE_MEM_HANDLERS_8BIT(20)
DECLARE_MEM_HANDLERS_8BIT(21)
DECLARE_MEM_HANDLERS_8BIT(24)
#define change_pc16(pc)			change_pc_generic(pc, 16, 0, cpu_setopbase16)
#define change_pc17(pc) 		change_pc_generic(pc, 17, 0, cpu_setopbase17)
#define change_pc20(pc)			change_pc_generic(pc, 20, 0, cpu_setopbase20)
#define change_pc21(pc)			change_pc_generic(pc, 21, 0, cpu_setopbase21)
#define change_pc24(pc)			change_pc_generic(pc, 24, 0, cpu_setopbase24)

/* ----- declare 16-bit bigendian handlers ----- */
DECLARE_MEM_HANDLERS_16BIT_BE(16)
DECLARE_MEM_HANDLERS_16BIT_BE(18)
DECLARE_MEM_HANDLERS_16BIT_BE(24)
DECLARE_MEM_HANDLERS_16BIT_BE(32)
#define change_pc16bew(pc)		change_pc_generic(pc, 16, 1, cpu_setopbase16bew)
#define change_pc18bew(pc)      change_pc_generic(pc, 18, 1, cpu_setopbase18bew)
#define change_pc24bew(pc)		change_pc_generic(pc, 24, 1, cpu_setopbase24bew)
#define change_pc32bew(pc)		change_pc_generic(pc, 32, 1, cpu_setopbase32bew)

/* ----- declare 16-bit littleendian handlers ----- */
DECLARE_MEM_HANDLERS_16BIT_LE(16)
DECLARE_MEM_HANDLERS_16BIT_LE(17)
DECLARE_MEM_HANDLERS_16BIT_LE(24)
DECLARE_MEM_HANDLERS_16BIT_LE(29)
DECLARE_MEM_HANDLERS_16BIT_LE(32)
#define change_pc16lew(pc)		change_pc_generic(pc, 16, 1, cpu_setopbase16lew)
#define change_pc17lew(pc)		change_pc_generic(pc, 17, 1, cpu_setopbase17lew)
#define change_pc24lew(pc)		change_pc_generic(pc, 24, 1, cpu_setopbase24lew)
#define change_pc29lew(pc)		change_pc_generic(pc, 29, 1, cpu_setopbase29lew)
#define change_pc32lew(pc)		change_pc_generic(pc, 32, 1, cpu_setopbase32lew)

/* ----- declare 32-bit bigendian handlers ----- */
DECLARE_MEM_HANDLERS_32BIT_BE(24)
DECLARE_MEM_HANDLERS_32BIT_BE(29)
DECLARE_MEM_HANDLERS_32BIT_BE(32)
#define change_pc24bedw(pc)		change_pc_generic(pc, 24, 2, cpu_setopbase24bedw)
#define change_pc29bedw(pc)		change_pc_generic(pc, 29, 2, cpu_setopbase29bedw)
#define change_pc32bedw(pc)		change_pc_generic(pc, 32, 2, cpu_setopbase32bedw)

/* ----- declare 32-bit littleendian handlers ----- */
DECLARE_MEM_HANDLERS_32BIT_LE(24)
DECLARE_MEM_HANDLERS_32BIT_LE(26)
DECLARE_MEM_HANDLERS_32BIT_LE(29)
DECLARE_MEM_HANDLERS_32BIT_LE(32)
#define change_pc24ledw(pc)		change_pc_generic(pc, 24, 2, cpu_setopbase24ledw)
#define change_pc26ledw(pc)		change_pc_generic(pc, 26, 2, cpu_setopbase26ledw)
#define change_pc29ledw(pc)		change_pc_generic(pc, 29, 2, cpu_setopbase29ledw)
#define change_pc32ledw(pc)		change_pc_generic(pc, 32, 2, cpu_setopbase32ledw)

/* ----- declare pdp1 handler ----- */
DECLARE_MEM_HANDLERS_32BIT_BE(18)
#define change_pc28bedw(pc)		change_pc_generic(pc, 18, 2, cpu_setopbase18bedw)


/***************************************************************************

	Function prototypes for core readport/writeport routines

***************************************************************************/

/* ----- declare 8-bit handlers ----- */
DECLARE_PORT_HANDLERS_8BIT(16)

/* ----- declare 16-bit bigendian handlers ----- */
DECLARE_PORT_HANDLERS_16BIT_BE(16)

/* ----- declare 16-bit littleendian handlers ----- */
DECLARE_PORT_HANDLERS_16BIT_LE(16)
DECLARE_PORT_HANDLERS_16BIT_LE(24)

/* ----- declare 32-bit bigendian handlers ----- */
DECLARE_PORT_HANDLERS_32BIT_BE(16)

/* ----- declare 32-bit littleendian handlers ----- */
DECLARE_PORT_HANDLERS_32BIT_LE(16)
DECLARE_PORT_HANDLERS_32BIT_LE(24)
DECLARE_PORT_HANDLERS_32BIT_LE(32)


/***************************************************************************

	Function prototypes for core memory functions

***************************************************************************/

/* ----- memory setup function ----- */
int			memory_init(void);
void		memory_shutdown(void);
void		memory_set_context(int activecpu);
void		memory_set_unmap_value(data32_t value);

/* ----- dynamic bank handlers ----- */
void		memory_set_bankhandler_r(int bank, offs_t offset, mem_read_handler handler);
void		memory_set_bankhandler_w(int bank, offs_t offset, mem_write_handler handler);

/* ----- opcode base control ---- */
opbase_handler memory_set_opbase_handler(int cpunum, opbase_handler function);

/* ----- separate opcode/data encryption helpers ---- */
void		memory_set_opcode_base(int cpunum, void *base);
void		memory_set_encrypted_opcode_range(int cpunum, offs_t min_address,offs_t max_address);
extern offs_t encrypted_opcode_start[],encrypted_opcode_end[];

/* ----- return a base pointer to memory ---- */
void *		memory_find_base(int cpunum, offs_t offset);
void *		memory_get_read_ptr(int cpunum, offs_t offset);
void *		memory_get_write_ptr(int cpunum, offs_t offset);

/* ----- dynamic memory mapping ----- */
data8_t *	install_mem_read_handler    (int cpunum, offs_t start, offs_t end, mem_read_handler handler);
data16_t *	install_mem_read16_handler  (int cpunum, offs_t start, offs_t end, mem_read16_handler handler);
data32_t *	install_mem_read32_handler  (int cpunum, offs_t start, offs_t end, mem_read32_handler handler);
data8_t *	install_mem_write_handler   (int cpunum, offs_t start, offs_t end, mem_write_handler handler);
data16_t *	install_mem_write16_handler (int cpunum, offs_t start, offs_t end, mem_write16_handler handler);
data32_t *	install_mem_write32_handler (int cpunum, offs_t start, offs_t end, mem_write32_handler handler);

/* ----- dynamic port mapping ----- */
void		install_port_read_handler   (int cpunum, offs_t start, offs_t end, port_read_handler handler);
void		install_port_read16_handler (int cpunum, offs_t start, offs_t end, port_read16_handler handler);
void		install_port_read32_handler (int cpunum, offs_t start, offs_t end, port_read32_handler handler);
void		install_port_write_handler  (int cpunum, offs_t start, offs_t end, port_write_handler handler);
void		install_port_write16_handler(int cpunum, offs_t start, offs_t end, port_write16_handler handler);
void		install_port_write32_handler(int cpunum, offs_t start, offs_t end, port_write32_handler handler);



/***************************************************************************

	Global variables

***************************************************************************/

extern UINT8 			opcode_entry;		/* current entry for opcode fetching */
extern UINT8 *			OP_ROM;				/* opcode ROM base */
extern UINT8 *			OP_RAM;				/* opcode RAM base */
extern offs_t			OP_MEM_MIN;			/* opcode memory minimum */
extern offs_t			OP_MEM_MAX;			/* opcode memory maximum */
extern UINT8 *			cpu_bankbase[];		/* array of bank bases */
extern UINT8 *			readmem_lookup;		/* pointer to the readmem lookup table */
extern offs_t			mem_amask;			/* memory address mask */
extern struct ExtMemory	ext_memory[];		/* externally-allocated memory */



/***************************************************************************

	Helper macros

***************************************************************************/

/* ----- 16/32-bit memory accessing ----- */
#define COMBINE_DATA(varptr)		(*(varptr) = (*(varptr) & mem_mask) | (data & ~mem_mask))

/* ----- 16-bit memory accessing ----- */
#define ACCESSING_LSB16				((mem_mask & 0x00ff) == 0)
#define ACCESSING_MSB16				((mem_mask & 0xff00) == 0)
#define ACCESSING_LSB				ACCESSING_LSB16
#define ACCESSING_MSB				ACCESSING_MSB16

/* ----- 32-bit memory accessing ----- */
#define ACCESSING_LSW32				((mem_mask & 0x0000ffff) == 0)
#define ACCESSING_MSW32				((mem_mask & 0xffff0000) == 0)
#define ACCESSING_LSB32				((mem_mask & 0x000000ff) == 0)
#define ACCESSING_MSB32				((mem_mask & 0xff000000) == 0)

/* ----- opcode range safety checks ----- */
#if CPUREADOP_SAFETY_NONE
#define address_is_unsafe(A)		(0)
#elif CPUREADOP_SAFETY_PARTIAL
#define address_is_unsafe(A)		(UNEXPECTED((A) > OP_MEM_MAX))
#elif CPUREADOP_SAFETY_FULL
#define address_is_unsafe(A)		((UNEXPECTED((A) < OP_MEM_MIN) || UNEXPECTED((A) > OP_MEM_MAX)))
#else
#error Must set either CPUREADOP_SAFETY_NONE, CPUREADOP_SAFETY_PARTIAL or CPUREADOP_SAFETY_FULL
#endif

/* ----- safe opcode and opcode argument reading ----- */
data8_t		cpu_readop_safe(offs_t offset);
data16_t	cpu_readop16_safe(offs_t offset);
data32_t	cpu_readop32_safe(offs_t offset);
data8_t		cpu_readop_arg_safe(offs_t offset);
data16_t	cpu_readop_arg16_safe(offs_t offset);
data32_t	cpu_readop_arg32_safe(offs_t offset);

/* ----- unsafe opcode and opcode argument reading ----- */
#define cpu_readop_unsafe(A)		(OP_ROM[(A) & mem_amask])
#define cpu_readop16_unsafe(A)		(*(data16_t *)&OP_ROM[(A) & mem_amask])
#define cpu_readop32_unsafe(A)		(*(data32_t *)&OP_ROM[(A) & mem_amask])
#define cpu_readop_arg_unsafe(A)	(OP_RAM[(A) & mem_amask])
#define cpu_readop_arg16_unsafe(A)	(*(data16_t *)&OP_RAM[(A) & mem_amask])
#define cpu_readop_arg32_unsafe(A)	(*(data32_t *)&OP_RAM[(A) & mem_amask])

/* ----- opcode and opcode argument reading ----- */
void activecpu_set_op_base(unsigned val);
static INLINE data8_t  cpu_readop(offs_t A)		{ if (address_is_unsafe(A)) { activecpu_set_op_base(A); } return cpu_readop_unsafe(A); }
static INLINE data16_t cpu_readop16(offs_t A)		{ if (address_is_unsafe(A)) { activecpu_set_op_base(A); } return cpu_readop16_unsafe(A); }
static INLINE data32_t cpu_readop32(offs_t A)		{ if (address_is_unsafe(A)) { activecpu_set_op_base(A); } return cpu_readop32_unsafe(A); }
static INLINE data8_t  cpu_readop_arg(offs_t A)	{ if (address_is_unsafe(A)) { activecpu_set_op_base(A); } return cpu_readop_arg_unsafe(A); }
static INLINE data16_t cpu_readop_arg16(offs_t A)	{ if (address_is_unsafe(A)) { activecpu_set_op_base(A); } return cpu_readop_arg16_unsafe(A); }
static INLINE data32_t cpu_readop_arg32(offs_t A)	{ if (address_is_unsafe(A)) { activecpu_set_op_base(A); } return cpu_readop_arg32_unsafe(A); }

/* ----- bank switching for CPU cores ----- */
#define change_pc_generic(pc,abits,minbits,setop)										\
do {																					\
	if (readmem_lookup[LEVEL1_INDEX((pc) & mem_amask,abits,minbits)] != opcode_entry)	\
		setop(pc);																		\
} while (0)																				\


/* ----- forces the next branch to generate a call to the opbase handler ----- */
#define catch_nextBranch()			(opcode_entry = 0xff)

/* ----- bank switching macro ----- */
#define cpu_setbank(bank, base) 														\
do {																					\
	if (bank >= STATIC_BANK1 && bank <= STATIC_BANKMAX)									\
	{																					\
		cpu_bankbase[bank] = (UINT8 *)(base);											\
		if (opcode_entry == bank && cpu_getactivecpu() >= 0)							\
		{																				\
			opcode_entry = 0xff;														\
			activecpu_set_op_base(activecpu_get_pc_byte());											\
		}																				\
	}																					\
} while (0)



#ifdef __cplusplus
}
#endif

#endif	/* !_MEMORY_H */

