/***************************************************************************

  memory.c

  Functions which handle the CPU memory and I/O port access.

  Caveats:

  * The install_mem/port_*_handler functions are only intended to be
	called at driver init time. Do not call them after this time.

  * If your driver executes an opcode which crosses a bank-switched
	boundary, it will pull the wrong data out of memory. Although not
	a common case, you may need to revert to memcpy to work around this.
	See machine/tnzs.c for an example.

***************************************************************************/

#include "driver.h"
#include "osd_cpu.h"
#include "state.h"

#include <stdarg.h>


/*#define MEM_DUMP*/
/*#define CHECK_MASKS*/



/***************************************************************************

	Basic theory of memory handling:

	An address with up to 32 bits is passed to a memory handler. First,
	the non-significant bits are removed from the bottom; for example,
	a 16-bit memory handler doesn't care about the low bit, so that is
	removed.

	Next, the address is broken into two halves, an upper half and a
	lower half. The number of bits in each half varies based on the
	total number of address bits. The upper half is then used as an
	index into the base_lookup table.

	If the value pulled from the table is within the range 192-255, then
	the lower half of the address is needed to resolve the final handler.
	The value from the table (192-255) is combined with the lower address
	bits to form an index into a subtable.

	Table values in the range 0-31 are reserved for internal handling
	(such as RAM, ROM, NOP, and banking). Table values between 32 and 192
	are assigned dynamically at startup.

***************************************************************************/

/* macros for the profiler */
#define MEMREADSTART			profiler_mark(PROFILER_MEMREAD);
#define MEMREADEND(ret)			{ profiler_mark(PROFILER_END); return ret; }
#define MEMWRITESTART			profiler_mark(PROFILER_MEMWRITE);
#define MEMWRITEEND(ret)		{ (ret); profiler_mark(PROFILER_END); return; }

#define DATABITS_TO_SHIFT(d)	(((d) == 32) ? 2 : ((d) == 16) ? 1 : 0)

/* helper macros */
#define HANDLER_IS_RAM(h)		((FPTR)(h) == STATIC_RAM)
#define HANDLER_IS_ROM(h)		((FPTR)(h) == STATIC_ROM)
#define HANDLER_IS_RAMROM(h)	((FPTR)(h) == STATIC_RAMROM)
#define HANDLER_IS_NOP(h)		((FPTR)(h) == STATIC_NOP)
#define HANDLER_IS_BANK(h)		((FPTR)(h) >= STATIC_BANK1 && (FPTR)(h) <= STATIC_BANKMAX)
#define HANDLER_IS_STATIC(h)	((FPTR)(h) < STATIC_COUNT)

#define HANDLER_TO_BANK(h)		((FPTR)(h))
#define BANK_TO_HANDLER(b)		((void *)(b))


/*-------------------------------------------------
	TYPE DEFINITIONS
-------------------------------------------------*/

struct bank_data
{
	UINT8 				used;				/* is this bank used? */
	UINT8 				cpunum;				/* the CPU it is used for */
	offs_t 				base;				/* the base offset */
	offs_t				readoffset;			/* original base offset for reads */
	offs_t				writeoffset;		/* original base offset for writes */
};

struct handler_data
{
	void *				handler;			/* function pointer for handler */
	offs_t				offset;				/* base offset for handler */
	offs_t				top;				/* maximum offset for handler */
};

struct table_data
{
	UINT8 *				table;				/* pointer to base of table */
	UINT8 				subtable_count;		/* number of subtables used */
	UINT8 				subtable_alloc;		/* number of subtables allocated */
	struct handler_data *handlers;			/* pointer to which set of handlers */
};

struct memport_data
{
	int					cpunum;				/* CPU index */
	int					abits;				/* address bits */
	int 				dbits;				/* data bits */
	int					ebits;				/* effective address bits */
	offs_t				mask;				/* address mask */
	struct table_data	read;				/* memory read lookup table */
	struct table_data	write;				/* memory write lookup table */
};

struct cpu_data
{
	void *				rambase;			/* RAM base pointer */
	size_t				ramlength;			/* RAM length pointer */
	opbase_handler 		opbase;				/* opcode base handler */

	void *				op_ram;				/* dynamic RAM base pointer */
	void *				op_rom;				/* dynamic ROM base pointer */
	offs_t				op_mem_min;			/* dynamic ROM/RAM min */
	offs_t				op_mem_max;			/* dynamic ROM/RAM max */
	UINT8		 		opcode_entry;		/* opcode base handler */

	struct memport_data	mem;				/* memory tables */
	struct memport_data	port;				/* port tables */
};

struct memory_address_table
{
	int 				bits;				/* address bits */
	read8_handler		handler;			/* handler associated with that */
};



/*-------------------------------------------------
	GLOBAL VARIABLES
-------------------------------------------------*/

static int					cur_context;					/* current CPU context */

UINT8 *						OP_ROM;							/* opcode ROM base */
UINT8 *						OP_RAM;							/* opcode RAM base */
offs_t						OP_MEM_MIN;						/* opcode memory minimum */
offs_t						OP_MEM_MAX;						/* opcode memory maximum */
UINT8		 				opcode_entry;					/* opcode readmem entry */

UINT8 *						readmem_lookup;					/* memory read lookup table */
static UINT8 *				writemem_lookup;				/* memory write lookup table */
static UINT8 *				readport_lookup;				/* port read lookup table */
static UINT8 *				writeport_lookup;				/* port write lookup table */

offs_t						mem_amask;						/* memory address mask */
static offs_t				port_amask;						/* port address mask */

UINT8 *						cpu_bankbase[STATIC_COUNT];		/* array of bank bases */
int ext_entries = 0;										/* number of entries ext_memory[] entries used */
struct ExtMemory			ext_memory[MAX_EXT_MEMORY];		/* externally-allocated memory */

static data32_t				unmap_value;					/* unmapped memory value */

static opbase_handler		opbasefunc;						/* opcode base override */

static struct handler_data 	rmemhandler8[ENTRY_COUNT];		/* 8-bit memory read handlers */
static struct handler_data 	rmemhandler16[ENTRY_COUNT];		/* 16-bit memory read handlers */
static struct handler_data 	rmemhandler32[ENTRY_COUNT];		/* 32-bit memory read handlers */
static struct handler_data 	wmemhandler8[ENTRY_COUNT];		/* 8-bit memory write handlers */
static struct handler_data 	wmemhandler16[ENTRY_COUNT];		/* 16-bit memory write handlers */
static struct handler_data 	wmemhandler32[ENTRY_COUNT];		/* 32-bit memory write handlers */

static struct handler_data 	rporthandler8[ENTRY_COUNT];		/* 8-bit port read handlers */
static struct handler_data 	rporthandler16[ENTRY_COUNT];	/* 16-bit port read handlers */
static struct handler_data 	rporthandler32[ENTRY_COUNT];	/* 32-bit port read handlers */
static struct handler_data 	wporthandler8[ENTRY_COUNT];		/* 8-bit port write handlers */
static struct handler_data 	wporthandler16[ENTRY_COUNT];	/* 16-bit port write handlers */
static struct handler_data 	wporthandler32[ENTRY_COUNT];	/* 32-bit port write handlers */

static read8_handler 		rmemhandler8s[STATIC_COUNT];	/* copy of 8-bit static read memory handlers */
static write8_handler 		wmemhandler8s[STATIC_COUNT];	/* copy of 8-bit static write memory handlers */

static struct cpu_data 		cpudata[MAX_CPU];				/* data gathered for each CPU */
static struct bank_data 	bankdata[MAX_BANKS];			/* data gathered for each bank */

offs_t encrypted_opcode_start[MAX_CPU],encrypted_opcode_end[MAX_CPU];


/*-------------------------------------------------
	PROTOTYPES
-------------------------------------------------*/

static int CLIB_DECL fatalerror(const char *string, ...);
static UINT8 get_handler_index(struct handler_data *table, void *handler, offs_t start);
static UINT8 alloc_new_subtable(const struct memport_data *memport, struct table_data *tabledata, UINT8 previous_value);
static void populate_table(struct memport_data *memport, int iswrite, offs_t start, offs_t stop, UINT8 handler);
static void *assign_dynamic_bank(int cpunum, offs_t start);
static void install_mem_handler(struct memport_data *memport, int iswrite, offs_t start, offs_t end, void *handler);
static void install_port_handler(struct memport_data *memport, int iswrite, offs_t start, offs_t end, void *handler);
static void set_static_handler(int idx,
		read8_handler r8handler, read16_handler r16handler, read32_handler r32handler,
		write8_handler w8handler, write16_handler w16handler, write32_handler w32handler);
static int init_cpudata(void);
static int init_memport(int cpunum, struct memport_data *data, int abits, int dbits, int ismemory);
static int verify_memory(void);
static int verify_ports(void);
static int allocate_memory(void);
static int populate_memory(void);
static int populate_ports(void);
static void register_banks(void);
static int mem_address_bits_of_cpu(int cpunum);
static int port_address_bits_of_cpu(int cpunum);
static int init_static(void);

#ifdef MEM_DUMP
static void mem_dump(void);
#endif
#ifdef CHECK_MASKS
static void verify_masks(void);
#endif



/*-------------------------------------------------
	memory_init - initialize the memory system
-------------------------------------------------*/

int memory_init(void)
{
#ifdef CHECK_MASKS
	verify_masks();
#endif

	/* no current context to start */
	cur_context = -1;
	unmap_value = 0;

	/* init the static handlers */
	if (!init_static())
		return 0;

	/* init the CPUs */
	if (!init_cpudata())
		return 0;

	/* verify the memory handlers and check banks */
	if (!verify_memory())
		return 0;
	if (!verify_ports())
		return 0;

	/* allocate memory for sparse address spaces */
	if (!allocate_memory())
		return 0;

	/* then fill in the tables */
	if (!populate_memory())
		return 0;
	if (!populate_ports())
		return 0;

	register_banks();

#ifdef MEM_DUMP
	/* dump the final memory configuration */
	mem_dump();
#endif

	return 1;
}


/*-------------------------------------------------
	memory_shutdown - free memory
-------------------------------------------------*/

void memory_shutdown(void)
{
	struct ExtMemory *ext;
	int ext_entry;
	int cpunum;

	/* free all the tables */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++ )
	{
		if (cpudata[cpunum].mem.read.table)
			free(cpudata[cpunum].mem.read.table);
		if (cpudata[cpunum].mem.write.table)
			free(cpudata[cpunum].mem.write.table);
		if (cpudata[cpunum].port.read.table)
			free(cpudata[cpunum].port.read.table);
		if (cpudata[cpunum].port.write.table)
			free(cpudata[cpunum].port.write.table);
	}
	memset(&cpudata, 0, sizeof(cpudata));

	/* free all the external memory */
	ext = ext_memory;
	for( ext_entry = 0; ext_entry < ext_entries; ext_entry++ )
	{
		free( ext->data );
		ext++;
	}
	memset(ext_memory, 0, sizeof(ext_memory));
	ext_entries = 0;
}


/*-------------------------------------------------
	memory_set_opcode_base - set the base of
	ROM
-------------------------------------------------*/

void memory_set_opcode_base(int cpunum, void *base)
{
	if (cur_context == cpunum)
	{
		OP_ROM = base;
		OP_MEM_MIN = (offs_t) 0x00000000;
		OP_MEM_MAX = (offs_t) 0x7fffffff;
	}
	else
	{
		cpudata[cpunum].op_rom = base;
		cpudata[cpunum].op_mem_min = (offs_t) 0x00000000;
		cpudata[cpunum].op_mem_max = (offs_t) 0x7fffffff;
	}
}


void memory_set_encrypted_opcode_range(int cpunum,offs_t min_address,offs_t max_address)
{
	encrypted_opcode_start[cpunum] = min_address;
	encrypted_opcode_end[cpunum] = max_address;
}


/*-------------------------------------------------
	memory_set_context - set the memory context
-------------------------------------------------*/

void memory_set_context(int activecpu)
{
	/* remember dynamic RAM/ROM */
	if (cur_context != -1)
	{
		cpudata[cur_context].op_ram = OP_RAM;
		cpudata[cur_context].op_rom = OP_ROM;
		cpudata[cur_context].op_mem_min = OP_MEM_MIN;
		cpudata[cur_context].op_mem_max = OP_MEM_MAX;
		cpudata[cur_context].opcode_entry = opcode_entry;
	}
	cur_context = activecpu;

	cpu_bankbase[STATIC_RAM] = cpudata[activecpu].rambase;
	OP_RAM = cpudata[activecpu].op_ram;
	OP_ROM = cpudata[activecpu].op_rom;
	OP_MEM_MIN = cpudata[activecpu].op_mem_min;
	OP_MEM_MAX = cpudata[activecpu].op_mem_max;
	opcode_entry = opcode_entry;

	readmem_lookup = cpudata[activecpu].mem.read.table;
	writemem_lookup = cpudata[activecpu].mem.write.table;
	readport_lookup = cpudata[activecpu].port.read.table;
	writeport_lookup = cpudata[activecpu].port.write.table;

	mem_amask = cpudata[activecpu].mem.mask;
	port_amask = cpudata[activecpu].port.mask;

	opbasefunc = cpudata[activecpu].opbase;
}


/*-------------------------------------------------
	memory_set_unmap_value - set the unmapped
	memory value
-------------------------------------------------*/

void memory_set_unmap_value(data32_t value)
{
	unmap_value = value;
}


/*-------------------------------------------------
	memory_set_bankhandler_r - set readmemory
	handler for bank memory (8-bit only!)
-------------------------------------------------*/

void memory_set_bankhandler_r(int bank, offs_t offset, mem_read_handler handler)
{
	/* determine the new offset */
	if (HANDLER_IS_RAM(handler) || HANDLER_IS_ROM(handler))
		rmemhandler8[bank].offset = 0 - offset, handler = (mem_read_handler)STATIC_RAM;
	else if (HANDLER_IS_BANK(handler))
		rmemhandler8[bank].offset = bankdata[HANDLER_TO_BANK(handler)].readoffset - offset;
	else
		rmemhandler8[bank].offset = bankdata[bank].readoffset - offset;

	/* set the new handler */
	if (HANDLER_IS_STATIC(handler))
		handler = rmemhandler8s[(FPTR)handler];
	rmemhandler8[bank].handler = (void *)handler;
}


/*-------------------------------------------------
	memory_set_bankhandler_w - set writememory
	handler for bank memory (8-bit only!)
-------------------------------------------------*/

void memory_set_bankhandler_w(int bank, offs_t offset, mem_write_handler handler)
{
	/* determine the new offset */
	if (HANDLER_IS_RAM(handler) || HANDLER_IS_ROM(handler) || HANDLER_IS_RAMROM(handler))
		wmemhandler8[bank].offset = 0 - offset;
	else if (HANDLER_IS_BANK(handler))
		wmemhandler8[bank].offset = bankdata[HANDLER_TO_BANK(handler)].writeoffset - offset;
	else
		wmemhandler8[bank].offset = bankdata[bank].writeoffset - offset;

	/* set the new handler */
	if (HANDLER_IS_STATIC(handler))
		handler = wmemhandler8s[(FPTR)handler];
	wmemhandler8[bank].handler = (void *)handler;
}


/*-------------------------------------------------
	memory_set_opbase_handler - change op-code
	memory base
-------------------------------------------------*/

opbase_handler memory_set_opbase_handler(int cpunum, opbase_handler function)
{
	opbase_handler old = cpudata[cpunum].opbase;
	cpudata[cpunum].opbase = function;
	if (cpunum == cpu_getactivecpu())
		opbasefunc = function;
	return old;
}


/*-------------------------------------------------
	install_mem_read_handler - install dynamic
	read handler for 8-bit case
-------------------------------------------------*/

data8_t *install_mem_read_handler(int cpunum, offs_t start, offs_t end, mem_read_handler handler)
{
	/* sanity check */
	if (cpudata[cpunum].mem.dbits != 8)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "fatal: install_mem_read_handler called on %d-bit cpu\n",cpudata[cpunum].mem.dbits);
		exit(1);
	}

	/* install the handler */
	install_mem_handler(&cpudata[cpunum].mem, 0, start, end, (void *)handler);
#ifdef MEM_DUMP
	/* dump the new memory configuration */
	mem_dump();
#endif
	return memory_find_base(cpunum, start);
}


/*-------------------------------------------------
	install_mem_read16_handler - install dynamic
	read handler for 16-bit case
-------------------------------------------------*/

data16_t *install_mem_read16_handler(int cpunum, offs_t start, offs_t end, mem_read16_handler handler)
{
	/* sanity check */
	if (cpudata[cpunum].mem.dbits != 16)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "fatal: install_mem_read16_handler called on %d-bit cpu\n",cpudata[cpunum].mem.dbits);
		exit(1);
	}

	/* install the handler */
	install_mem_handler(&cpudata[cpunum].mem, 0, start, end, (void *)handler);
#ifdef MEM_DUMP
	/* dump the new memory configuration */
	mem_dump();
#endif
	return memory_find_base(cpunum, start);
}


/*-------------------------------------------------
	install_mem_read32_handler - install dynamic
	read handler for 32-bit case
-------------------------------------------------*/

data32_t *install_mem_read32_handler(int cpunum, offs_t start, offs_t end, mem_read32_handler handler)
{
	/* sanity check */
	if (cpudata[cpunum].mem.dbits != 32)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "fatal: install_mem_read32_handler called on %d-bit cpu\n",cpudata[cpunum].mem.dbits);
		exit(1);
	}

	/* install the handler */
	install_mem_handler(&cpudata[cpunum].mem, 0, start, end, (void *)handler);
#ifdef MEM_DUMP
	/* dump the new memory configuration */
	mem_dump();
#endif
	return memory_find_base(cpunum, start);
}


/*-------------------------------------------------
	install_mem_write_handler - install dynamic
	read handler for 8-bit case
-------------------------------------------------*/

data8_t *install_mem_write_handler(int cpunum, offs_t start, offs_t end, mem_write_handler handler)
{
	/* sanity check */
	if (cpudata[cpunum].mem.dbits != 8)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "fatal: install_mem_write_handler called on %d-bit cpu\n",cpudata[cpunum].mem.dbits);
		exit(1);
	}

	/* install the handler */
	install_mem_handler(&cpudata[cpunum].mem, 1, start, end, (void *)handler);
#ifdef MEM_DUMP
	/* dump the new memory configuration */
	mem_dump();
#endif
	return memory_find_base(cpunum, start);
}


/*-------------------------------------------------
	install_mem_write16_handler - install dynamic
	read handler for 16-bit case
-------------------------------------------------*/

data16_t *install_mem_write16_handler(int cpunum, offs_t start, offs_t end, mem_write16_handler handler)
{
	/* sanity check */
	if (cpudata[cpunum].mem.dbits != 16)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "fatal: install_mem_write16_handler called on %d-bit cpu\n",cpudata[cpunum].mem.dbits);
		exit(1);
	}

	/* install the handler */
	install_mem_handler(&cpudata[cpunum].mem, 1, start, end, (void *)handler);
#ifdef MEM_DUMP
	/* dump the new memory configuration */
	mem_dump();
#endif
	return memory_find_base(cpunum, start);
}


/*-------------------------------------------------
	install_mem_write32_handler - install dynamic
	read handler for 32-bit case
-------------------------------------------------*/

data32_t *install_mem_write32_handler(int cpunum, offs_t start, offs_t end, mem_write32_handler handler)
{
	/* sanity check */
	if (cpudata[cpunum].mem.dbits != 32)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "fatal: install_mem_write32_handler called on %d-bit cpu\n",cpudata[cpunum].mem.dbits);
		exit(1);
	}

	/* install the handler */
	install_mem_handler(&cpudata[cpunum].mem, 1, start, end, (void *)handler);
#ifdef MEM_DUMP
	/* dump the new memory configuration */
	mem_dump();
#endif
	return memory_find_base(cpunum, start);
}


/*-------------------------------------------------
	install_port_read_handler - install dynamic
	read handler for 8-bit case
-------------------------------------------------*/

void install_port_read_handler(int cpunum, offs_t start, offs_t end, port_read_handler handler)
{
	/* sanity check */
	if (cpudata[cpunum].port.dbits != 8)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "fatal: install_port_read_handler called on %d-bit cpu\n",cpudata[cpunum].port.dbits);
		exit(1);
	}

	/* install the handler */
	install_port_handler(&cpudata[cpunum].port, 0, start, end, (void *)handler);
#ifdef MEM_DUMP
	/* dump the new memory configuration */
	mem_dump();
#endif
}


/*-------------------------------------------------
	install_port_read16_handler - install dynamic
	read handler for 16-bit case
-------------------------------------------------*/

void install_port_read16_handler(int cpunum, offs_t start, offs_t end, port_read16_handler handler)
{
	/* sanity check */
	if (cpudata[cpunum].port.dbits != 16)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "fatal: install_port_read16_handler called on %d-bit cpu\n",cpudata[cpunum].port.dbits);
		exit(1);
	}

	/* install the handler */
	install_port_handler(&cpudata[cpunum].port, 0, start, end, (void *)handler);
#ifdef MEM_DUMP
	/* dump the new memory configuration */
	mem_dump();
#endif
}


/*-------------------------------------------------
	install_port_read32_handler - install dynamic
	read handler for 32-bit case
-------------------------------------------------*/

void install_port_read32_handler(int cpunum, offs_t start, offs_t end, port_read32_handler handler)
{
	/* sanity check */
	if (cpudata[cpunum].port.dbits != 32)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "fatal: install_port_read32_handler called on %d-bit cpu\n",cpudata[cpunum].port.dbits);
		exit(1);
	}

	/* install the handler */
	install_port_handler(&cpudata[cpunum].port, 0, start, end, (void *)handler);
#ifdef MEM_DUMP
	/* dump the new memory configuration */
	mem_dump();
#endif
}


/*-------------------------------------------------
	install_port_write_handler - install dynamic
	read handler for 8-bit case
-------------------------------------------------*/

void install_port_write_handler(int cpunum, offs_t start, offs_t end, port_write_handler handler)
{
	/* sanity check */
	if (cpudata[cpunum].port.dbits != 8)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "fatal: install_port_write_handler called on %d-bit cpu\n",cpudata[cpunum].port.dbits);
		exit(1);
	}

	/* install the handler */
	install_port_handler(&cpudata[cpunum].port, 1, start, end, (void *)handler);
#ifdef MEM_DUMP
	/* dump the new memory configuration */
	mem_dump();
#endif
}


/*-------------------------------------------------
	install_port_write16_handler - install dynamic
	read handler for 16-bit case
-------------------------------------------------*/

void install_port_write16_handler(int cpunum, offs_t start, offs_t end, port_write16_handler handler)
{
	/* sanity check */
	if (cpudata[cpunum].port.dbits != 16)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "fatal: install_port_write16_handler called on %d-bit cpu\n",cpudata[cpunum].port.dbits);
		exit(1);
	}

	/* install the handler */
	install_port_handler(&cpudata[cpunum].port, 1, start, end, (void *)handler);
#ifdef MEM_DUMP
	/* dump the new memory configuration */
	mem_dump();
#endif
}


/*-------------------------------------------------
	install_port_write32_handler - install dynamic
	read handler for 32-bit case
-------------------------------------------------*/

void install_port_write32_handler(int cpunum, offs_t start, offs_t end, port_write32_handler handler)
{
	/* sanity check */
	if (cpudata[cpunum].port.dbits != 32)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "fatal: install_port_write32_handler called on %d-bit cpu\n",cpudata[cpunum].port.dbits);
		exit(1);
	}

	/* install the handler */
	install_port_handler(&cpudata[cpunum].port, 1, start, end, (void *)handler);
#ifdef MEM_DUMP
	/* dump the new memory configuration */
	mem_dump();
#endif
}


/*-------------------------------------------------
	fatalerror - display an error message and
	exit immediately
-------------------------------------------------*/

int CLIB_DECL fatalerror(const char *string, ...)
{
	va_list arg;
	va_start(arg, string);
	log_cb(RETRO_LOG_ERROR, string, arg);
	va_end(arg);
	exit(1);
	return 0;
}


/*-------------------------------------------------
	memory_find_base - return a pointer to the
	base of RAM associated with the given CPU
	and offset
-------------------------------------------------*/

void *memory_find_base(int cpunum, offs_t offset)
{
	int ext_entry;
	int region = REGION_CPU1 + cpunum;
	struct ExtMemory *ext;

	/* look in external memory first */
	ext = ext_memory;
	for( ext_entry = 0; ext_entry < ext_entries; ext_entry++ )
	{
		if (ext->region == region && ext->start <= offset && ext->end >= offset)
		{
			return (void *)((UINT8 *)ext->data + (offset - ext->start));
		}
		ext++;
	}

	return (UINT8 *)cpudata[cpunum].rambase + offset;
}


/*-------------------------------------------------
	memory_get_read_ptr - return a pointer to the
	base of RAM associated with the given CPU
	and offset
-------------------------------------------------*/

void *memory_get_read_ptr(int cpunum, offs_t offset)
{
	struct memport_data *memport = &cpudata[cpunum].mem;
	struct handler_data *handlist = (memport->dbits == 32) ? rmemhandler32 : (memport->dbits == 16) ? rmemhandler16 : rmemhandler8;
	UINT8 minbits = memport->abits - memport->ebits;
	UINT8 entry;

	/* perform the lookup */
	offset &= memport->mask;
	entry = memport->read.table[LEVEL1_INDEX(offset, memport->abits, minbits)];
	if (entry >= SUBTABLE_BASE)
		entry = memport->read.table[LEVEL2_INDEX(entry, offset, memport->abits, minbits)];

	/* 8-bit case: RAM/ROM/RAMROM */
	if (entry > STATIC_RAM || (minbits == 0 && entry != STATIC_RAM))
		return NULL;
	offset -= handlist[entry].offset;
	return &cpu_bankbase[entry][offset];
}


/*-------------------------------------------------
	memory_get_write_ptr - return a pointer to the
	base of RAM associated with the given CPU
	and offset
-------------------------------------------------*/

void *memory_get_write_ptr(int cpunum, offs_t offset)
{
	struct memport_data *memport = &cpudata[cpunum].mem;
	struct handler_data *handlist = (memport->dbits == 32) ? wmemhandler32 : (memport->dbits == 16) ? wmemhandler16 : wmemhandler8;
	UINT8 minbits = memport->abits - memport->ebits;
	UINT8 entry;

	/* perform the lookup */
	offset &= memport->mask;
	entry = memport->write.table[LEVEL1_INDEX(offset, memport->abits, minbits)];
	if (entry >= SUBTABLE_BASE)
		entry = memport->write.table[LEVEL2_INDEX(entry, offset, memport->abits, minbits)];

	/* 8-bit case: RAM/ROM/RAMROM */
	if (entry > STATIC_RAM || (minbits == 0 && entry != STATIC_RAM))
		return NULL;
	offset -= handlist[entry].offset;
	return &cpu_bankbase[entry][offset];
}


/*-------------------------------------------------
	get_handler_index - finds the index of a
	handler, or allocates a new one as necessary
-------------------------------------------------*/

UINT8 get_handler_index(struct handler_data *table, void *handler, offs_t start)
{
	int i;

	/* all static handlers are hardcoded */
	if (HANDLER_IS_STATIC(handler))
		return (FPTR)handler;

	/* otherwise, we have to search */
	for (i = STATIC_COUNT; i < SUBTABLE_BASE; i++)
	{
		if (table[i].handler == NULL)
		{
			table[i].handler = handler;
			table[i].offset = start;
		}
		if (table[i].handler == handler && table[i].offset == start)
			return i;
	}
	return 0;
}


/*-------------------------------------------------
	alloc_new_subtable - allocates more space
	for a new subtable
-------------------------------------------------*/

UINT8 alloc_new_subtable(const struct memport_data *memport, struct table_data *tabledata, UINT8 previous_value)
{
	int l1bits = LEVEL1_BITS(memport->ebits);
	int l2bits = LEVEL2_BITS(memport->ebits);

	/* make sure we don't run out */
	if (tabledata->subtable_count + 1 == SUBTABLE_COUNT)
		fatalerror("error: ran out of memory subtables\n");


#ifdef __LIBRETRO__ /* HACK: Don't use realloc*/
    if(tabledata->subtable_alloc < SUBTABLE_COUNT)
    {
        tabledata->subtable_alloc = SUBTABLE_COUNT;
    }
#else
	/* allocate more memory if we need to */
	if (tabledata->subtable_count <= tabledata->subtable_alloc)
	{
		tabledata->subtable_alloc += SUBTABLE_ALLOC;
		tabledata->table = realloc(tabledata->table, (1 << l1bits) + (tabledata->subtable_alloc << l2bits));
		if (!tabledata->table)
			fatalerror("error: ran out of memory allocating memory subtable\n");
	}
#endif

	/* initialize the table entries */
	memset(&tabledata->table[(1 << l1bits) + (tabledata->subtable_count << l2bits)], previous_value, 1 << l2bits);

	/* return the new index */
	return SUBTABLE_BASE + tabledata->subtable_count++;
}


/*-------------------------------------------------
	populate_table - assign a memory handler to
	a range of addresses
-------------------------------------------------*/

void populate_table(struct memport_data *memport, int iswrite, offs_t start, offs_t stop, UINT8 handler)
{
	struct table_data *tabledata = iswrite ? &memport->write : &memport->read;
	int minbits = DATABITS_TO_SHIFT(memport->dbits);
	int l1bits = LEVEL1_BITS(memport->ebits);
	int l2bits = LEVEL2_BITS(memport->ebits);
	offs_t l2mask = LEVEL2_MASK(memport->ebits);
	offs_t l1start = start >> (l2bits + minbits);
	offs_t l2start = (start >> minbits) & l2mask;
	offs_t l1stop = stop >> (l2bits + minbits);
	offs_t l2stop = (stop >> minbits) & l2mask;
	UINT8 subindex;

	/* sanity check */
	if (start > stop)
		return;

	/* set the base for non RAM/ROM cases */
	if (handler != STATIC_RAM && handler != STATIC_ROM && handler != STATIC_RAMROM)
	{
		tabledata->handlers[handler].offset = start;
		tabledata->handlers[handler].top = stop;
	}

	/* remember the base for banks */
	if (handler >= STATIC_BANK1 && handler <= STATIC_BANKMAX)
	{
		if (iswrite)
			bankdata[handler].writeoffset = start;
		else
			bankdata[handler].readoffset = start;
	}

	/* handle the starting edge if it's not on a block boundary */
	if (l2start != 0)
	{
		/* get the subtable index */
		subindex = tabledata->table[l1start];
		if (subindex < SUBTABLE_BASE)
			subindex = tabledata->table[l1start] = alloc_new_subtable(memport, tabledata, subindex);
		subindex &= SUBTABLE_MASK;

		/* if the start and stop end within the same block, handle that */
		if (l1start == l1stop)
		{
			memset(&tabledata->table[(1 << l1bits) + (subindex << l2bits) + l2start], handler, l2stop - l2start + 1);
			return;
		}

		/* otherwise, fill until the end */
		memset(&tabledata->table[(1 << l1bits) + (subindex << l2bits) + l2start], handler, (1 << l2bits) - l2start);
		if (l1start != (offs_t)~0) l1start++;
	}

	/* handle the trailing edge if it's not on a block boundary */
	if (l2stop != l2mask)
	{
		/* get the subtable index */
		subindex = tabledata->table[l1stop];
		if (subindex < SUBTABLE_BASE)
			subindex = tabledata->table[l1stop] = alloc_new_subtable(memport, tabledata, subindex);
		subindex &= SUBTABLE_MASK;

		/* fill from the beginning */
		memset(&tabledata->table[(1 << l1bits) + (subindex << l2bits)], handler, l2stop + 1);

		/* if the start and stop end within the same block, handle that */
		if (l1start == l1stop)
			return;
		if (l1stop != 0) l1stop--;
	}

	/* now fill in the middle tables */
	if (l1start <= l1stop)
		memset(&tabledata->table[l1start], handler, l1stop - l1start + 1);
}


/*-------------------------------------------------
	assign_dynamic_bank - finds a free or exact
	matching bank
-------------------------------------------------*/

void *assign_dynamic_bank(int cpunum, offs_t start)
{
	int bank;

	/* special case: never assign a dynamic bank to an offset that */
	/* intersects the CPU's region; always use RAM for that */
	if (start < memory_region_length(REGION_CPU1 + cpunum))
		return (void *)STATIC_RAM;

	/* loop over banks, searching for an exact match or an empty */
	for (bank = 1; bank <= MAX_BANKS; bank++)
		if (!bankdata[bank].used || (bankdata[bank].cpunum == cpunum && bankdata[bank].base == start))
		{
			bankdata[bank].used = 1;
			bankdata[bank].cpunum = cpunum;
			bankdata[bank].base = start;
			return BANK_TO_HANDLER(bank);
		}

	/* if we got here, we failed */
	fatalerror("cpu #%d: ran out of banks for sparse memory regions!\n", cpunum);
	return NULL;
}


/*-------------------------------------------------
	install_mem_handler - installs a handler for
	memory operatinos
-------------------------------------------------*/

void install_mem_handler(struct memport_data *memport, int iswrite, offs_t start, offs_t end, void *handler)
{
	struct table_data *tabledata = iswrite ? &memport->write : &memport->read;
	UINT8 idx;

	/* translate ROM and RAMROM to RAM here for read cases */
	if (!iswrite)
		if (HANDLER_IS_ROM(handler) || HANDLER_IS_RAMROM(handler))
			handler = (void *)MRA_RAM;

	/* assign banks for sparse memory spaces */
	if (IS_SPARSE(memport->abits) && HANDLER_IS_RAM(handler))
		handler = (void *)assign_dynamic_bank(memport->cpunum, start);

	/* set the handler */
	idx = get_handler_index(tabledata->handlers, handler, start);
	populate_table(memport, iswrite, start, end, idx);

	/* if this is a bank, set the bankbase as well */
	if (HANDLER_IS_BANK(handler))
		cpu_bankbase[HANDLER_TO_BANK(handler)] = memory_find_base(memport->cpunum, start);
}


/*-------------------------------------------------
	install_port_handler - installs a handler for
	port operatinos
-------------------------------------------------*/

void install_port_handler(struct memport_data *memport, int iswrite, offs_t start, offs_t end, void *handler)
{
	struct table_data *tabledata = iswrite ? &memport->write : &memport->read;
	UINT8 idx = get_handler_index(tabledata->handlers, handler, start);
	populate_table(memport, iswrite, start, end, idx);
}


/*-------------------------------------------------
	set_static_handler - handy shortcut for
	setting all 6 handlers for a given index
-------------------------------------------------*/

static void set_static_handler(int idx,
		read8_handler r8handler, read16_handler r16handler, read32_handler r32handler,
		write8_handler w8handler, write16_handler w16handler, write32_handler w32handler)
{
	rmemhandler8s[idx] = r8handler;
	wmemhandler8s[idx] = w8handler;

	rmemhandler8[idx].handler = (void *)r8handler;
	rmemhandler16[idx].handler = (void *)r16handler;
	rmemhandler32[idx].handler = (void *)r32handler;
	wmemhandler8[idx].handler = (void *)w8handler;
	wmemhandler16[idx].handler = (void *)w16handler;
	wmemhandler32[idx].handler = (void *)w32handler;

	rporthandler8[idx].handler = (void *)r8handler;
	rporthandler16[idx].handler = (void *)r16handler;
	rporthandler32[idx].handler = (void *)r32handler;
	wporthandler8[idx].handler = (void *)w8handler;
	wporthandler16[idx].handler = (void *)w16handler;
	wporthandler32[idx].handler = (void *)w32handler;
}


/*-------------------------------------------------
	init_cpudata - initialize the cpudata
	structure for each CPU
-------------------------------------------------*/

static int init_cpudata(void)
{
	int cpunum;

	/* zap the cpudata structure */
	memset(&cpudata, 0, sizeof(cpudata));

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		int cputype = Machine->drv->cpu[cpunum].cpu_type;

		/* set the RAM/ROM base */
		cpudata[cpunum].rambase = cpudata[cpunum].op_ram = cpudata[cpunum].op_rom = memory_region(REGION_CPU1 + cpunum);
		cpudata[cpunum].op_mem_max = cpudata[cpunum].ramlength = memory_region_length(REGION_CPU1 + cpunum);
		cpudata[cpunum].op_mem_min = 0;
		cpudata[cpunum].opcode_entry = STATIC_ROM;
		cpudata[cpunum].opbase = NULL;
		encrypted_opcode_start[cpunum] = 0;
		encrypted_opcode_end[cpunum] = 0;

		/* initialize the readmem and writemem tables */
		if (!init_memport(cpunum, &cpudata[cpunum].mem, mem_address_bits_of_cpu(cputype), cpunum_databus_width(cpunum), 1))
			return 0;

		/* initialize the readport and writeport tables */
		if (!init_memport(cpunum, &cpudata[cpunum].port, port_address_bits_of_cpu(cputype), cpunum_databus_width(cpunum), 0))
			return 0;

#if HAS_Z80
		/* Z80 port mask kludge */
		if (cputype == CPU_Z80)
			if (!(Machine->drv->cpu[cpunum].cpu_flags & CPU_16BIT_PORT))
				cpudata[cpunum].port.mask = 0xff;
#endif
#if HAS_DRZ80
		if (cputype == CPU_DRZ80)
			if (!(Machine->drv->cpu[cpunum].cpu_flags & CPU_16BIT_PORT))
				cpudata[cpunum].port.mask = 0xff;
#endif
#if HAS_Z180
		/* Z180 port mask kludge */
		if (cputype == CPU_Z180)
			if (!(Machine->drv->cpu[cpunum].cpu_flags & CPU_16BIT_PORT))
				cpudata[cpunum].port.mask = 0xff;
#endif
	}
	return 1;
}


/*-------------------------------------------------
	init_memport - initialize the mem/port data
	structure
-------------------------------------------------*/

static int init_memport(int cpunum, struct memport_data *data, int abits, int dbits, int ismemory)
{
	/* determine the address and data bits */
	data->cpunum = cpunum;
	data->abits = abits;
	data->dbits = dbits;
	data->ebits = abits - DATABITS_TO_SHIFT(dbits);
	data->mask = 0xffffffffUL >> (32 - abits);

	/* allocate memory */
#ifdef __LIBRETRO__ /* HACK: Don't use realloc*/
	data->read.table = malloc(1024*1024);
	data->write.table = malloc(1024*1024);
#else
	data->read.table = malloc(1 << LEVEL1_BITS(data->ebits));
	data->write.table = malloc(1 << LEVEL1_BITS(data->ebits));
#endif

	if (!data->read.table)
		return fatalerror("cpu #%d couldn't allocate read table\n", cpunum);
	if (!data->write.table)
		return fatalerror("cpu #%d couldn't allocate write table\n", cpunum);

	/* initialize everything to unmapped */
	memset(data->read.table, STATIC_UNMAP, 1 << LEVEL1_BITS(data->ebits));
	memset(data->write.table, STATIC_UNMAP, 1 << LEVEL1_BITS(data->ebits));

	/* initialize the pointers to the handlers */
	if (ismemory)
	{
		data->read.handlers = (dbits == 32) ? rmemhandler32 : (dbits == 16) ? rmemhandler16 : rmemhandler8;
		data->write.handlers = (dbits == 32) ? wmemhandler32 : (dbits == 16) ? wmemhandler16 : wmemhandler8;
	}
	else
	{
		data->read.handlers = (dbits == 32) ? rporthandler32 : (dbits == 16) ? rporthandler16 : rporthandler8;
		data->write.handlers = (dbits == 32) ? wporthandler32 : (dbits == 16) ? wporthandler16 : wporthandler8;
	}
	return 1;
}


/*-------------------------------------------------
	verify_memory - verify the memory structs
	and track which banks are referenced
-------------------------------------------------*/

static int verify_memory(void)
{
	int cpunum;

	/* zap the bank data */
	memset(&bankdata, 0, sizeof(bankdata));

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		const struct Memory_ReadAddress *mra = Machine->drv->cpu[cpunum].memory_read;
		const struct Memory_WriteAddress *mwa = Machine->drv->cpu[cpunum].memory_write;
		UINT32 width;
		int bank;

		/* determine the desired width */
		switch (cpunum_databus_width(cpunum))
		{
			case 8:		width = MEMPORT_WIDTH_8;	break;
			case 16:	width = MEMPORT_WIDTH_16;	break;
			case 32:	width = MEMPORT_WIDTH_32;	break;
			default:	return fatalerror("cpu #%d has invalid memory width!\n", cpunum);
		}

		/* verify the read handlers */
		if (mra)
		{
			/* verify the MEMPORT_READ_START header */
			if (mra->start == MEMPORT_MARKER && mra->end != 0)
			{
				if ((mra->end & MEMPORT_TYPE_MASK) != MEMPORT_TYPE_MEM)
					return fatalerror("cpu #%d has port handlers in place of memory read handlers!\n", cpunum);
				if ((mra->end & MEMPORT_DIRECTION_MASK) != MEMPORT_DIRECTION_READ)
					return fatalerror("cpu #%d has memory write handlers in place of memory read handlers!\n", cpunum);
				if ((mra->end & MEMPORT_WIDTH_MASK) != width)
					return fatalerror("cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n", cpunum,cpunum_databus_width(cpunum),mra->end);
				mra++;
			}

			/* track banks used */
			for ( ; !IS_MEMPORT_END(mra); mra++)
				if (!IS_MEMPORT_MARKER(mra) && HANDLER_IS_BANK(mra->handler))
				{
					bank = HANDLER_TO_BANK(mra->handler);
					bankdata[bank].used = 1;
					bankdata[bank].cpunum = -1;
				}
		}

		/* verify the write handlers */
		if (mwa)
		{
			/* verify the MEMPORT_WRITE_START header */
			if (mwa->start == MEMPORT_MARKER && mwa->end != 0)
			{
				if ((mwa->end & MEMPORT_TYPE_MASK) != MEMPORT_TYPE_MEM)
					return fatalerror("cpu #%d has port handlers in place of memory write handlers!\n", cpunum);
				if ((mwa->end & MEMPORT_DIRECTION_MASK) != MEMPORT_DIRECTION_WRITE)
					return fatalerror("cpu #%d has memory read handlers in place of memory write handlers!\n", cpunum);
				if ((mwa->end & MEMPORT_WIDTH_MASK) != width)
					return fatalerror("cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n", cpunum,cpunum_databus_width(cpunum),mwa->end);
				mwa++;
			}

			/* track banks used */
			for (; !IS_MEMPORT_END(mwa); mwa++)
				if (!IS_MEMPORT_MARKER(mwa) && HANDLER_IS_BANK(mwa->handler))
				{
					bank = HANDLER_TO_BANK(mwa->handler);
					bankdata[bank].used = 1;
					bankdata[bank].cpunum = -1;
				}
				mwa++;
		}
	}
	return 1;
}


/*-------------------------------------------------
	verify_ports - verify the port structs
-------------------------------------------------*/

static int verify_ports(void)
{
	int cpunum;

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		const struct IO_ReadPort *mra = Machine->drv->cpu[cpunum].port_read;
		const struct IO_WritePort *mwa = Machine->drv->cpu[cpunum].port_write;
		UINT32 width;

		/* determine the desired width */
		switch (cpunum_databus_width(cpunum))
		{
			case 8:		width = MEMPORT_WIDTH_8;	break;
			case 16:	width = MEMPORT_WIDTH_16;	break;
			case 32:	width = MEMPORT_WIDTH_32;	break;
			default:	return fatalerror("cpu #%d has invalid memory width!\n", cpunum);
		}

		/* verify the read handlers */
		if (mra)
		{
			/* verify the PORT_READ_START header */
			if (mra->start == MEMPORT_MARKER && mra->end != 0)
			{
				if ((mra->end & MEMPORT_TYPE_MASK) != MEMPORT_TYPE_IO)
					return fatalerror("cpu #%d has memory handlers in place of I/O read handlers!\n", cpunum);
				if ((mra->end & MEMPORT_DIRECTION_MASK) != MEMPORT_DIRECTION_READ)
					return fatalerror("cpu #%d has port write handlers in place of port read handlers!\n", cpunum);
				if ((mra->end & MEMPORT_WIDTH_MASK) != width)
					return fatalerror("cpu #%d uses wrong data width port handlers! (width = %d, memory = %08x)\n", cpunum,cpunum_databus_width(cpunum),mra->end);
			}
		}

		/* verify the write handlers */
		if (mwa)
		{
			/* verify the PORT_WRITE_START header */
			if (mwa->start == MEMPORT_MARKER && mwa->end != 0)
			{
				if ((mwa->end & MEMPORT_TYPE_MASK) != MEMPORT_TYPE_IO)
					return fatalerror("cpu #%d has memory handlers in place of I/O write handlers!\n", cpunum);
				if ((mwa->end & MEMPORT_DIRECTION_MASK) != MEMPORT_DIRECTION_WRITE)
					return fatalerror("cpu #%d has port read handlers in place of port write handlers!\n", cpunum);
				if ((mwa->end & MEMPORT_WIDTH_MASK) != width)
					return fatalerror("cpu #%d uses wrong data width port handlers! (width = %d, memory = %08x)\n", cpunum,cpunum_databus_width(cpunum),mwa->end);
			}
		}
	}
	return 1;
}


/*-------------------------------------------------
	needs_ram - returns true if a given type
	of memory needs RAM backing it
-------------------------------------------------*/

static int needs_ram(int cpunum, void *handler)
{
	/* RAM, ROM, and banks always need RAM */
	if (HANDLER_IS_RAM(handler) || HANDLER_IS_ROM(handler) || HANDLER_IS_RAMROM(handler) || HANDLER_IS_BANK(handler))
		return 1;

	/* NOPs never need RAM */
	else if (HANDLER_IS_NOP(handler))
		return 0;

	/* otherwise, we only need RAM for sparse memory spaces */
	else
		return IS_SPARSE(cpudata[cpunum].mem.abits);
}


/*-------------------------------------------------
	allocate_memory - allocate memory for
	sparse CPU address spaces
-------------------------------------------------*/

static int allocate_memory(void)
{
	struct ExtMemory *ext = ext_memory;
	int cpunum;

	ext_entries = 0;

	/* don't do it for drivers that don't have ROM (MESS needs this) */
	if (Machine->gamedrv->rom == 0)
		return 1;

	/* loop over all CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		int region = REGION_CPU1 + cpunum;
		int region_length = memory_region(region) ? memory_region_length(region) : 0;
		int size = region_length;

		/* keep going until we break out */
		while (1)
		{
			const struct Memory_ReadAddress *mra = Machine->drv->cpu[cpunum].memory_read;
			const struct Memory_WriteAddress *mwa = Machine->drv->cpu[cpunum].memory_write;
			offs_t lowest = ~0, end, lastend;

			/* find the base of the lowest memory region that extends past the end */
			for (mra = Machine->drv->cpu[cpunum].memory_read; !IS_MEMPORT_END(mra); mra++)
				if (!IS_MEMPORT_MARKER(mra))
					if (mra->end >= size && mra->start < lowest && needs_ram(cpunum, (void *)mra->handler))
						lowest = mra->start;

			for (mwa = Machine->drv->cpu[cpunum].memory_write; !IS_MEMPORT_END(mwa); mwa++)
				if (!IS_MEMPORT_MARKER(mwa))
					if (mwa->end >= size && mwa->start < lowest && (mwa->base || needs_ram(cpunum, (void *)mwa->handler)))
						lowest = mwa->start;

			/* done if nothing found */
			if (lowest == ~0)
				break;

			/* now loop until we find the end of this contiguous block of memory */
			lastend = ~0;
			end = lowest;
			while (end != lastend)
			{
				lastend = end;

				/* find the end of the contiguous block of memory */
				for (mra = Machine->drv->cpu[cpunum].memory_read; !IS_MEMPORT_END(mra); mra++)
					if (!IS_MEMPORT_MARKER(mra))
						if (mra->start <= end+1 && mra->end > end && needs_ram(cpunum, (void *)mra->handler))
							end = mra->end;

				for (mwa = Machine->drv->cpu[cpunum].memory_write; !IS_MEMPORT_END(mwa); mwa++)
					if (!IS_MEMPORT_MARKER(mwa))
						if (mwa->start <= end+1 && mwa->end > end && (mwa->base || needs_ram(cpunum, (void *)mwa->handler)))
							end = mwa->end;
			}

			ext_entries++;
			if( ext_entries > MAX_EXT_MEMORY )
			{
				return fatalerror("MAX_EXT_MEMORY too small (%d)\n", ext_entries);
			}

			/* fill in the data structure */
			ext->start = lowest;
			ext->end = end;
			ext->region = region;

			/* allocate memory */
			ext->data = malloc(end+1 - lowest);
			if (!ext->data)
				fatalerror("malloc(%d) failed (lowest: %x - end: %x)\n", end + 1 - lowest, lowest, end);

			/* reset the memory */
			memset(ext->data, 0, end+1 - lowest);

			/* prepare for the next loop */
			size = ext->end + 1;

			/* check for wraparound */
			if (size < ext->end)
				break;

			ext++;
		}
	}
	return 1;
}


/*-------------------------------------------------
	populate_memory - populate the memory mapping
	tables with entries
-------------------------------------------------*/

static int populate_memory(void)
{
	int cpunum;

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		const struct Memory_ReadAddress *mra, *mra_start = Machine->drv->cpu[cpunum].memory_read;
		const struct Memory_WriteAddress *mwa, *mwa_start = Machine->drv->cpu[cpunum].memory_write;

		/* install the read handlers */
		if (mra_start)
		{
			/* first find the end and check for address bits */
			for (mra = mra_start; !IS_MEMPORT_END(mra); mra++)
				if (IS_MEMPORT_MARKER(mra) && (mra->end & MEMPORT_ABITS_MASK))
					cpudata[cpunum].mem.mask = 0xffffffffUL >> (32 - (mra->end & MEMPORT_ABITS_VAL_MASK));

			/* then work backwards */
			for (mra--; mra >= mra_start; mra--)
				if (!IS_MEMPORT_MARKER(mra))
					install_mem_handler(&cpudata[cpunum].mem, 0, mra->start, mra->end, (void *)mra->handler);
		}

		/* install the write handlers */
		if (mwa_start)
		{
			/* first find the end and check for address bits */
			for (mwa = mwa_start; !IS_MEMPORT_END(mwa); mwa++)
				if (IS_MEMPORT_MARKER(mwa) && (mwa->end & MEMPORT_ABITS_MASK))
					cpudata[cpunum].mem.mask = 0xffffffffUL >> (32 - (mwa->end & MEMPORT_ABITS_VAL_MASK));

			/* then work backwards */
			for (mwa--; mwa >= mwa_start; mwa--)
				if (!IS_MEMPORT_MARKER(mwa))
				{
					install_mem_handler(&cpudata[cpunum].mem, 1, mwa->start, mwa->end, (void *)mwa->handler);
					if (mwa->base) *mwa->base = memory_find_base(cpunum, mwa->start);
					if (mwa->size) *mwa->size = mwa->end - mwa->start + 1;
				}
		}
	}
	return 1;
}


/*-------------------------------------------------
	populate_ports - populate the port mapping
	tables with entries
-------------------------------------------------*/

static int populate_ports(void)
{
	int cpunum;

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		const struct IO_ReadPort *mra, *mra_start = Machine->drv->cpu[cpunum].port_read;
		const struct IO_WritePort *mwa, *mwa_start = Machine->drv->cpu[cpunum].port_write;

		/* install the read handlers */
		if (mra_start)
		{
			/* first find the end and check for address bits */
			for (mra = mra_start; !IS_MEMPORT_END(mra); mra++)
				if (IS_MEMPORT_MARKER(mra) && (mra->end & MEMPORT_ABITS_MASK))
					cpudata[cpunum].port.mask = 0xffffffffUL >> (32 - (mra->end & MEMPORT_ABITS_VAL_MASK));

			/* then work backwards */
			for (mra--; mra != mra_start; mra--)
				if (!IS_MEMPORT_MARKER(mra))
					install_port_handler(&cpudata[cpunum].port, 0, mra->start, mra->end, (void *)mra->handler);
		}

		/* install the write handlers */
		if (mwa_start)
		{
			/* first find the end and check for address bits */
			for (mwa = mwa_start; !IS_MEMPORT_END(mwa); mwa++)
				if (IS_MEMPORT_MARKER(mwa) && (mwa->end & MEMPORT_ABITS_MASK))
					cpudata[cpunum].port.mask = 0xffffffffUL >> (32 - (mwa->end & MEMPORT_ABITS_VAL_MASK));

			/* then work backwards */
			for (mwa--; mwa != mwa_start; mwa--)
				if (!IS_MEMPORT_MARKER(mwa))
					install_port_handler(&cpudata[cpunum].port, 1, mwa->start, mwa->end, (void *)mwa->handler);
		}
	}
	return 1;
}


/*-------------------------------------------------
	register_banks - Registers all memory banks
    into the state save system
-------------------------------------------------*/
typedef struct rg_map_entry {
	struct rg_map_entry *next;
	UINT32 start;
	UINT32 end;
	int flags;
} rg_map_entry;

static rg_map_entry *rg_map = 0;

enum {
	RG_SAVE_READ  = 0x0001,
	RG_DROP_READ  = 0x0002,
	RG_READ_MASK  = 0x00ff,

	RG_SAVE_WRITE = 0x0100,
	RG_DROP_WRITE = 0x0200,
	RG_WRITE_MASK = 0xff00
};

static void rg_add_entry(UINT32 start, UINT32 end, int mode)
{
	rg_map_entry **cur;
	cur = &rg_map;
	while(*cur && ((*cur)->end < start))
		cur = &(*cur)->next;

	while(start <= end)
	{
		int mask;
		if(!*cur || ((*cur)->start > start))
		{
			rg_map_entry *e = malloc(sizeof(rg_map_entry));
			e->start = start;
			e->end = *cur && (*cur)->start <= end ? (*cur)->start - 1 : end;
			e->flags = mode;
			e->next = *cur;
			*cur = e;
			cur = &(*cur)->next;
			start = e->end + 1;

			/* check for wraparound */
			if(start == 0 || start > end)
				return;
		}

		if((*cur)->start < start)
		{
			rg_map_entry *e = malloc(sizeof(rg_map_entry));
			e->start = (*cur)->start;
			e->end = start - 1;
			e->flags = (*cur)->flags;
			e->next = *cur;
			(*cur)->start = start;
			*cur = e;
			cur = &(*cur)->next;
		}

		if((*cur)->end > end)
		{
			rg_map_entry *e = malloc(sizeof(rg_map_entry));
			e->start = start;
			e->end = end;
			e->flags = (*cur)->flags;
			e->next = *cur;
			(*cur)->start = end+1;
			*cur = e;
		}

		mask = 0;

		if (mode & RG_READ_MASK)
			mask |= RG_READ_MASK;
		if (mode & RG_WRITE_MASK)
			mask |= RG_WRITE_MASK;

		(*cur)->flags = ((*cur)->flags & ~mask) | mode;
		start = (*cur)->end + 1;
		cur = &(*cur)->next;

		/* check for wraparound */
		if (start == 0)
			break;
	}
}

static void rg_map_clear(void)
{
	rg_map_entry *e = rg_map;
	while(e)
	{
		rg_map_entry *n = e->next;
		free(e);
		e = n;
	}
	rg_map = 0;
}

static void register_zone(int cpunum, UINT32 start, UINT32 end)
{
	char name[256];
	sprintf (name, "%08x-%08x", start, end);
	switch (cpunum_databus_width(cpunum))
	{
	case 8:
		state_save_register_UINT8 ("memory", cpunum, name, memory_find_base(cpunum, start), end-start+1);
		break;
	case 16:
		state_save_register_UINT16("memory", cpunum, name, memory_find_base(cpunum, start), (end-start+1)/2);
		break;
	case 32:
		state_save_register_UINT32("memory", cpunum, name, memory_find_base(cpunum, start), (end-start+1)/4);
		break;
	}
}

void register_banks(void)
{
	int cpunum, i;
	int banksize[MAX_BANKS];
	int bankcpu[MAX_BANKS];

	for (i=0; i<MAX_BANKS; i++)
	{
		banksize[i] = 0;
		bankcpu[i] = -1;
	}

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		const struct Memory_ReadAddress *mra, *mra_start = Machine->drv->cpu[cpunum].memory_read;
		const struct Memory_WriteAddress *mwa, *mwa_start = Machine->drv->cpu[cpunum].memory_write;
		int bits = cpudata[cpunum].mem.abits;
/*		int width = cpunum_databus_width(cpunum);*/

		if (!IS_SPARSE(bits))
		{
			UINT32 size = memory_region_length(REGION_CPU1 + cpunum);
			if (size > (1<<bits))
				size = 1 << bits;
			rg_add_entry(0, size-1, RG_SAVE_READ|RG_SAVE_WRITE);
		}


		if (mra_start)
		{
			for (mra = mra_start; !IS_MEMPORT_END(mra); mra++);
			mra--;
			for (;mra != mra_start; mra--)
			{
				if (!IS_MEMPORT_MARKER (mra))
				{
					int mode;
					mem_read_handler h = mra->handler;
					if (!HANDLER_IS_STATIC (h))
						mode = RG_DROP_READ;
					else if (HANDLER_IS_RAM(h))
						mode = RG_SAVE_READ;
					else if (HANDLER_IS_ROM(h))
						mode = RG_DROP_READ;
					else if (HANDLER_IS_RAMROM(h))
						mode = RG_SAVE_READ;
					else if (HANDLER_IS_NOP(h))
						mode = RG_DROP_READ;
					else if (HANDLER_IS_BANK(h))
					{
						int size = mra->end-mra->start+1;
						if (banksize[HANDLER_TO_BANK(h)] < size)
							banksize[HANDLER_TO_BANK(h)] = size;
						bankcpu[HANDLER_TO_BANK(h)] = cpunum;
						mode = RG_DROP_READ;
					}
					else
						abort();
					rg_add_entry(mra->start, mra->end, mode);
				}
			}
		}
		if (mwa_start)
		{
			for (mwa = mwa_start; !IS_MEMPORT_END(mwa); mwa++);
			mwa--;
			for (;mwa != mwa_start; mwa--)
			{
				if (!IS_MEMPORT_MARKER (mwa))
				{
					int mode;
					mem_write_handler h = mwa->handler;
					if (!HANDLER_IS_STATIC (h))
						mode = mwa->base ? RG_SAVE_WRITE : RG_DROP_WRITE;
					else if (HANDLER_IS_RAM(h))
						mode = RG_SAVE_WRITE;
					else if (HANDLER_IS_ROM(h))
						mode = RG_DROP_WRITE;
					else if (HANDLER_IS_RAMROM(h))
						mode = RG_SAVE_WRITE;
					else if (HANDLER_IS_NOP(h))
						mode = RG_DROP_WRITE;
					else if (HANDLER_IS_BANK(h))
					{
						int size = mwa->end-mwa->start+1;
						if (banksize[HANDLER_TO_BANK(h)] < size)
							banksize[HANDLER_TO_BANK(h)] = size;
						bankcpu[HANDLER_TO_BANK(h)] = cpunum;
						mode = RG_DROP_WRITE;;
					}
					else
						abort();
					rg_add_entry(mwa->start, mwa->end, mode);
				}
			}
		}

		{
			rg_map_entry *e = rg_map;
			UINT32 start = 0, end = 0;
			int active = 0;
			while (e)
			{
				if(e && (e->flags & (RG_SAVE_READ|RG_SAVE_WRITE)))
				{
					if (!active)
					{
						active = 1;
						start = e->start;
					}
					end = e->end;
				}
				else if (active)
				{
					register_zone (cpunum, start, end);
					active = 0;
				}

				if (active && (!e->next || (e->end+1 != e->next->start)))
				{
					register_zone (cpunum, start, end);
					active = 0;
				}
				e = e->next;
			}
		}

		rg_map_clear();
	}

	for (i=0; i<MAX_BANKS; i++)
		if (banksize[i])
			switch (cpunum_databus_width(bankcpu[i]))
			{
			case 8:
				state_save_register_UINT8 ("bank", i, "ram",           cpu_bankbase[i], banksize[i]);
				break;
			case 16:
				state_save_register_UINT16("bank", i, "ram", (UINT16 *)cpu_bankbase[i], banksize[i]/2);
				break;
			case 32:
				state_save_register_UINT32("bank", i, "ram", (UINT32 *)cpu_bankbase[i], banksize[i]/4);
				break;
			}

}

/*-------------------------------------------------
	READBYTE - generic byte-sized read handler
-------------------------------------------------*/

#define READBYTE8(name,abits,lookup,handlist,mask)										\
data8_t name(offs_t address)															\
{																						\
	UINT8 entry;																		\
	MEMREADSTART																		\
																						\
	/* perform lookup */																\
	address &= mask;																	\
	entry = lookup[LEVEL1_INDEX(address,abits,0)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,0)];							\
																						\
	/* for compatibility with setbankhandler, 8-bit systems */							\
	/* must call handlers for banks */													\
	if (entry == STATIC_RAM)															\
		MEMREADEND(cpu_bankbase[STATIC_RAM][address])									\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		read8_handler handler = (read8_handler)handlist[entry].handler;					\
		MEMREADEND((*handler)(address - handlist[entry].offset))						\
	}																					\
	return 0;																			\
}																						\

#define READBYTE16BE(name,abits,lookup,handlist,mask)									\
data8_t name(offs_t address)															\
{																						\
	UINT8 entry;																		\
	MEMREADSTART																		\
																						\
	/* perform lookup */																\
	address &= mask;																	\
	entry = lookup[LEVEL1_INDEX(address,abits,1)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,1)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(cpu_bankbase[entry][BYTE_XOR_BE(address)])							\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (~address & 1);													\
		read16_handler handler = (read16_handler)handlist[entry].handler;				\
		MEMREADEND((*handler)(address >> 1, ~(0xff << shift)) >> shift)					\
	}																					\
	return 0;																			\
}																						\

#define READBYTE16LE(name,abits,lookup,handlist,mask)									\
data8_t name(offs_t address)															\
{																						\
	UINT8 entry;																		\
	MEMREADSTART																		\
																						\
	/* perform lookup */																\
	address &= mask;																	\
	entry = lookup[LEVEL1_INDEX(address,abits,1)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,1)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(cpu_bankbase[entry][BYTE_XOR_LE(address)])							\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (address & 1);													\
		read16_handler handler = (read16_handler)handlist[entry].handler;				\
		MEMREADEND((*handler)(address >> 1, ~(0xff << shift)) >> shift)					\
	}																					\
	return 0;																			\
}																						\

#define READBYTE32BE(name,abits,lookup,handlist,mask)									\
data8_t name(offs_t address)															\
{																						\
	UINT8 entry;																		\
	MEMREADSTART																		\
																						\
	/* perform lookup */																\
	address &= mask;																	\
	entry = lookup[LEVEL1_INDEX(address,abits,2)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,2)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(cpu_bankbase[entry][BYTE4_XOR_BE(address)])							\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (~address & 3);													\
		read32_handler handler = (read32_handler)handlist[entry].handler;				\
		MEMREADEND((*handler)(address >> 2, ~(0xff << shift)) >> shift) 				\
	}																					\
	return 0;																			\
}																						\

#define READBYTE32LE(name,abits,lookup,handlist,mask)									\
data8_t name(offs_t address)															\
{																						\
	UINT8 entry;																		\
	MEMREADSTART																		\
																						\
	/* perform lookup */																\
	address &= mask;																	\
	entry = lookup[LEVEL1_INDEX(address,abits,2)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,2)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(cpu_bankbase[entry][BYTE4_XOR_LE(address)])							\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (address & 3);													\
		read32_handler handler = (read32_handler)handlist[entry].handler;				\
		MEMREADEND((*handler)(address >> 2, ~(0xff << shift)) >> shift) 				\
	}																					\
	return 0;																			\
}																						\


/*-------------------------------------------------
	READWORD - generic word-sized read handler
	(16-bit and 32-bit aligned only!)
-------------------------------------------------*/

#define READWORD16(name,abits,lookup,handlist,mask)										\
data16_t name(offs_t address)															\
{																						\
	UINT8 entry;																		\
	MEMREADSTART																		\
																						\
	/* perform lookup */																\
	address &= mask & ~1;																\
	entry = lookup[LEVEL1_INDEX(address,abits,1)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,1)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(*(data16_t *)&cpu_bankbase[entry][address])							\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		read16_handler handler = (read16_handler)handlist[entry].handler;				\
		MEMREADEND((*handler)(address >> 1,0))										 	\
	}																					\
	return 0;																			\
}																						\

#define READWORD32BE(name,abits,lookup,handlist,mask)									\
data16_t name(offs_t address)															\
{																						\
	UINT8 entry;																		\
	MEMREADSTART																		\
																						\
	/* perform lookup */																\
	address &= mask & ~1;																\
	entry = lookup[LEVEL1_INDEX(address,abits,2)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,2)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(*(data16_t *)&cpu_bankbase[entry][WORD_XOR_BE(address)])				\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (~address & 2);													\
		read32_handler handler = (read32_handler)handlist[entry].handler;				\
		MEMREADEND((*handler)(address >> 2, ~(0xffff << shift)) >> shift)				\
	}																					\
	return 0;																			\
}																						\

#define READWORD32LE(name,abits,lookup,handlist,mask)									\
data16_t name(offs_t address)															\
{																						\
	UINT8 entry;																		\
	MEMREADSTART																		\
																						\
	/* perform lookup */																\
	address &= mask & ~1;																\
	entry = lookup[LEVEL1_INDEX(address,abits,2)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,2)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(*(data16_t *)&cpu_bankbase[entry][WORD_XOR_LE(address)])				\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (address & 2);													\
		read32_handler handler = (read32_handler)handlist[entry].handler;				\
		MEMREADEND((*handler)(address >> 2, ~(0xffff << shift)) >> shift)				\
	}																					\
	return 0;																			\
}																						\


/*-------------------------------------------------
	READLONG - generic dword-sized read handler
	(32-bit aligned only!)
-------------------------------------------------*/

#define READLONG32(name,abits,lookup,handlist,mask)										\
data32_t name(offs_t address)															\
{																						\
	UINT8 entry;																		\
	MEMREADSTART																		\
																						\
	/* perform lookup */																\
	address &= mask & ~3;																\
	entry = lookup[LEVEL1_INDEX(address,abits,2)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,2)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMREADEND(*(data32_t *)&cpu_bankbase[entry][address])							\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		read32_handler handler = (read32_handler)handlist[entry].handler;				\
		MEMREADEND((*handler)(address >> 2,0))										 	\
	}																					\
	return 0;																			\
}																						\


/*-------------------------------------------------
	WRITEBYTE - generic byte-sized write handler
-------------------------------------------------*/

#define WRITEBYTE8(name,abits,lookup,handlist,mask)										\
void name(offs_t address, data8_t data)													\
{																						\
	UINT8 entry;																		\
	MEMWRITESTART																		\
																						\
	/* perform lookup */																\
	address &= mask;																	\
	entry = lookup[LEVEL1_INDEX(address,abits,0)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,0)];							\
																						\
	/* for compatibility with setbankhandler, 8-bit systems */							\
	/* must call handlers for banks */													\
	if (entry == (FPTR)MRA_RAM)															\
		MEMWRITEEND(cpu_bankbase[STATIC_RAM][address] = data)							\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		write8_handler handler = (write8_handler)handlist[entry].handler;				\
		MEMWRITEEND((*handler)(address - handlist[entry].offset, data))					\
	}																					\
}																						\

#define WRITEBYTE16BE(name,abits,lookup,handlist,mask)									\
void name(offs_t address, data8_t data)													\
{																						\
	UINT8 entry;																		\
	MEMWRITESTART																		\
																						\
	/* perform lookup */																\
	address &= mask;																	\
	entry = lookup[LEVEL1_INDEX(address,abits,1)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,1)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(cpu_bankbase[entry][BYTE_XOR_BE(address)] = data)					\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (~address & 1);													\
		write16_handler handler = (write16_handler)handlist[entry].handler;				\
		MEMWRITEEND((*handler)(address >> 1, data << shift, ~(0xff << shift))) 			\
	}																					\
}																						\

#define WRITEBYTE16LE(name,abits,lookup,handlist,mask)									\
void name(offs_t address, data8_t data)													\
{																						\
	UINT8 entry;																		\
	MEMWRITESTART																		\
																						\
	/* perform lookup */																\
	address &= mask;																	\
	entry = lookup[LEVEL1_INDEX(address,abits,1)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,1)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(cpu_bankbase[entry][BYTE_XOR_LE(address)] = data)					\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (address & 1);													\
		write16_handler handler = (write16_handler)handlist[entry].handler;				\
		MEMWRITEEND((*handler)(address >> 1, data << shift, ~(0xff << shift)))			\
	}																					\
}																						\

#define WRITEBYTE32BE(name,abits,lookup,handlist,mask)									\
void name(offs_t address, data8_t data)													\
{																						\
	UINT8 entry;																		\
	MEMWRITESTART																		\
																						\
	/* perform lookup */																\
	address &= mask;																	\
	entry = lookup[LEVEL1_INDEX(address,abits,2)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,2)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(cpu_bankbase[entry][BYTE4_XOR_BE(address)] = data)					\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (~address & 3);													\
		write32_handler handler = (write32_handler)handlist[entry].handler;				\
		MEMWRITEEND((*handler)(address >> 2, data << shift, ~(0xff << shift))) 			\
	}																					\
}																						\

#define WRITEBYTE32LE(name,abits,lookup,handlist,mask)									\
void name(offs_t address, data8_t data)													\
{																						\
	UINT8 entry;																		\
	MEMWRITESTART																		\
																						\
	/* perform lookup */																\
	address &= mask;																	\
	entry = lookup[LEVEL1_INDEX(address,abits,2)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,2)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(cpu_bankbase[entry][BYTE4_XOR_LE(address)] = data)					\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (address & 3);													\
		write32_handler handler = (write32_handler)handlist[entry].handler;				\
		MEMWRITEEND((*handler)(address >> 2, data << shift, ~(0xff << shift))) 			\
	}																					\
}																						\


/*-------------------------------------------------
	WRITEWORD - generic word-sized write handler
	(16-bit and 32-bit aligned only!)
-------------------------------------------------*/

#define WRITEWORD16(name,abits,lookup,handlist,mask)									\
void name(offs_t address, data16_t data)												\
{																						\
	UINT8 entry;																		\
	MEMWRITESTART																		\
																						\
	/* perform lookup */																\
	address &= mask & ~1;																\
	entry = lookup[LEVEL1_INDEX(address,abits,1)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,1)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(*(data16_t *)&cpu_bankbase[entry][address] = data)					\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		write16_handler handler = (write16_handler)handlist[entry].handler;				\
		MEMWRITEEND((*handler)(address >> 1, data, 0))								 	\
	}																					\
}																						\

#define WRITEWORD32BE(name,abits,lookup,handlist,mask)									\
void name(offs_t address, data16_t data)												\
{																						\
	UINT8 entry;																		\
	MEMWRITESTART																		\
																						\
	/* perform lookup */																\
	address &= mask & ~1;																\
	entry = lookup[LEVEL1_INDEX(address,abits,2)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,2)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(*(data16_t *)&cpu_bankbase[entry][WORD_XOR_BE(address)] = data)		\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (~address & 2);													\
		write32_handler handler = (write32_handler)handlist[entry].handler;				\
		MEMWRITEEND((*handler)(address >> 2, data << shift, ~(0xffff << shift))) 		\
	}																					\
}																						\

#define WRITEWORD32LE(name,abits,lookup,handlist,mask)									\
void name(offs_t address, data16_t data)												\
{																						\
	UINT8 entry;																		\
	MEMWRITESTART																		\
																						\
	/* perform lookup */																\
	address &= mask & ~1;																\
	entry = lookup[LEVEL1_INDEX(address,abits,2)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,2)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(*(data16_t *)&cpu_bankbase[entry][WORD_XOR_LE(address)] = data)		\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (address & 2);													\
		write32_handler handler = (write32_handler)handlist[entry].handler;				\
		MEMWRITEEND((*handler)(address >> 2, data << shift, ~(0xffff << shift))) 		\
	}																					\
}																						\


/*-------------------------------------------------
	WRITELONG - dword-sized write handler
	(32-bit aligned only!)
-------------------------------------------------*/

#define WRITELONG32(name,abits,lookup,handlist,mask)									\
void name(offs_t address, data32_t data)												\
{																						\
	UINT8 entry;																		\
	MEMWRITESTART																		\
																						\
	/* perform lookup */																\
	address &= mask & ~3;																\
	entry = lookup[LEVEL1_INDEX(address,abits,2)];										\
	if (entry >= SUBTABLE_BASE)															\
		entry = lookup[LEVEL2_INDEX(entry,address,abits,2)];							\
																						\
	/* handle banks inline */															\
	address -= handlist[entry].offset;													\
	if (entry <= STATIC_RAM)															\
		MEMWRITEEND(*(data32_t *)&cpu_bankbase[entry][address] = data)					\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		write32_handler handler = (write32_handler)handlist[entry].handler;				\
		MEMWRITEEND((*handler)(address >> 2, data, 0))								 	\
	}																					\
}																						\


/*-------------------------------------------------
	SETOPBASE - generic opcode base changer
-------------------------------------------------*/

#define SETOPBASE(name,abits,minbits,table)												\
void name(offs_t pc)																	\
{																						\
	UINT8 *base;																		\
	UINT8 entry;																		\
																						\
	/* allow overrides */																\
	if (opbasefunc) 																	\
	{																					\
		pc = (*opbasefunc)(pc);															\
		if (pc == ~0)																	\
			return; 																	\
	}																					\
																						\
	/* perform the lookup */															\
	pc &= mem_amask;																	\
	entry = readmem_lookup[LEVEL1_INDEX(pc,abits,minbits)];								\
	if (entry >= SUBTABLE_BASE)															\
		entry = readmem_lookup[LEVEL2_INDEX(entry,pc,abits,minbits)];					\
	opcode_entry = entry;																\
																						\
	/* RAM/ROM/RAMROM */																\
	if (entry >= STATIC_RAM && entry <= STATIC_RAMROM)									\
		base = cpu_bankbase[STATIC_RAM];												\
																						\
	/* banked memory */																	\
	else if (entry >= STATIC_BANK1 && entry <= STATIC_RAM)								\
		base = cpu_bankbase[entry];														\
																						\
	/* other memory -- could be very slow! */											\
	else																				\
	{																					\
		logerror("cpu #%d (PC=%08X): warning - op-code execute on mapped I/O\n",		\
					cpu_getactivecpu(), activecpu_get_pc());							\
		/*base = memory_find_base(cpu_getactivecpu(), pc);*/							\
		return;																			\
	}																					\
																						\
	/* compute the adjusted base */														\
	OP_ROM = base - table[entry].offset + (OP_ROM - OP_RAM);							\
	OP_RAM = base - table[entry].offset;												\
	OP_MEM_MIN = table[entry].offset;													\
	OP_MEM_MAX = (entry >= STATIC_RAM && entry <= STATIC_RAMROM)						\
		? cpudata[cpu_getactivecpu()].ramlength - 1										\
		: table[entry].top;																\
}


/*-------------------------------------------------
	GENERATE_HANDLERS - macros to spew out all
	the handlers needed for a given memory type
-------------------------------------------------*/

#define GENERATE_HANDLERS_8BIT(type, abits) \
	    READBYTE8(cpu_read##type##abits,             abits, read##type##_lookup,  r##type##handler8,  type##_amask) \
	   WRITEBYTE8(cpu_write##type##abits,            abits, write##type##_lookup, w##type##handler8,  type##_amask)

#define GENERATE_HANDLERS_16BIT_BE(type, abits) \
	 READBYTE16BE(cpu_read##type##abits##bew,        abits, read##type##_lookup,  r##type##handler16, type##_amask) \
	   READWORD16(cpu_read##type##abits##bew_word,   abits, read##type##_lookup,  r##type##handler16, type##_amask) \
	WRITEBYTE16BE(cpu_write##type##abits##bew,       abits, write##type##_lookup, w##type##handler16, type##_amask) \
	  WRITEWORD16(cpu_write##type##abits##bew_word,  abits, write##type##_lookup, w##type##handler16, type##_amask)

#define GENERATE_HANDLERS_16BIT_LE(type, abits) \
	 READBYTE16LE(cpu_read##type##abits##lew,        abits, read##type##_lookup,  r##type##handler16, type##_amask) \
	   READWORD16(cpu_read##type##abits##lew_word,   abits, read##type##_lookup,  r##type##handler16, type##_amask) \
	WRITEBYTE16LE(cpu_write##type##abits##lew,       abits, write##type##_lookup, w##type##handler16, type##_amask) \
	  WRITEWORD16(cpu_write##type##abits##lew_word,  abits, write##type##_lookup, w##type##handler16, type##_amask)

#define GENERATE_HANDLERS_32BIT_BE(type, abits) \
	 READBYTE32BE(cpu_read##type##abits##bedw,       abits, read##type##_lookup,  r##type##handler32, type##_amask) \
	 READWORD32BE(cpu_read##type##abits##bedw_word,  abits, read##type##_lookup,  r##type##handler32, type##_amask) \
	   READLONG32(cpu_read##type##abits##bedw_dword, abits, read##type##_lookup,  r##type##handler32, type##_amask) \
	WRITEBYTE32BE(cpu_write##type##abits##bedw,      abits, write##type##_lookup, w##type##handler32, type##_amask) \
	WRITEWORD32BE(cpu_write##type##abits##bedw_word, abits, write##type##_lookup, w##type##handler32, type##_amask) \
	  WRITELONG32(cpu_write##type##abits##bedw_dword,abits, write##type##_lookup, w##type##handler32, type##_amask)

#define GENERATE_HANDLERS_32BIT_LE(type, abits) \
	 READBYTE32LE(cpu_read##type##abits##ledw,       abits, read##type##_lookup,  r##type##handler32, type##_amask) \
	 READWORD32LE(cpu_read##type##abits##ledw_word,  abits, read##type##_lookup,  r##type##handler32, type##_amask) \
	   READLONG32(cpu_read##type##abits##ledw_dword, abits, read##type##_lookup,  r##type##handler32, type##_amask) \
	WRITEBYTE32LE(cpu_write##type##abits##ledw,      abits, write##type##_lookup, w##type##handler32, type##_amask) \
	WRITEWORD32LE(cpu_write##type##abits##ledw_word, abits, write##type##_lookup, w##type##handler32, type##_amask) \
	  WRITELONG32(cpu_write##type##abits##ledw_dword,abits, write##type##_lookup, w##type##handler32, type##_amask)


/*-------------------------------------------------
	GENERATE_MEM_HANDLERS - memory handler
	variants of the GENERATE_HANDLERS
-------------------------------------------------*/

#define GENERATE_MEM_HANDLERS_8BIT(abits) \
GENERATE_HANDLERS_8BIT(mem, abits) \
SETOPBASE(cpu_setopbase##abits,           abits, 0, rmemhandler8)

#define GENERATE_MEM_HANDLERS_16BIT_BE(abits) \
GENERATE_HANDLERS_16BIT_BE(mem, abits) \
SETOPBASE(cpu_setopbase##abits##bew,      abits, 1, rmemhandler16)

#define GENERATE_MEM_HANDLERS_16BIT_LE(abits) \
GENERATE_HANDLERS_16BIT_LE(mem, abits) \
SETOPBASE(cpu_setopbase##abits##lew,      abits, 1, rmemhandler16)

#define GENERATE_MEM_HANDLERS_32BIT_BE(abits) \
GENERATE_HANDLERS_32BIT_BE(mem, abits) \
SETOPBASE(cpu_setopbase##abits##bedw,     abits, 2, rmemhandler32)

#define GENERATE_MEM_HANDLERS_32BIT_LE(abits) \
GENERATE_HANDLERS_32BIT_LE(mem, abits) \
SETOPBASE(cpu_setopbase##abits##ledw,     abits, 2, rmemhandler32)


/*-------------------------------------------------
	GENERATE_PORT_HANDLERS - port handler
	variants of the GENERATE_HANDLERS
-------------------------------------------------*/

#define GENERATE_PORT_HANDLERS_8BIT(abits) \
GENERATE_HANDLERS_8BIT(port, abits)

#define GENERATE_PORT_HANDLERS_16BIT_BE(abits) \
GENERATE_HANDLERS_16BIT_BE(port, abits)

#define GENERATE_PORT_HANDLERS_16BIT_LE(abits) \
GENERATE_HANDLERS_16BIT_LE(port, abits)

#define GENERATE_PORT_HANDLERS_32BIT_BE(abits) \
GENERATE_HANDLERS_32BIT_BE(port, abits)

#define GENERATE_PORT_HANDLERS_32BIT_LE(abits) \
GENERATE_HANDLERS_32BIT_LE(port, abits)


/*-------------------------------------------------
	the memory handlers we need to generate
-------------------------------------------------*/

GENERATE_MEM_HANDLERS_8BIT(16)
GENERATE_MEM_HANDLERS_8BIT(20)
GENERATE_MEM_HANDLERS_8BIT(21)
GENERATE_MEM_HANDLERS_8BIT(24)

GENERATE_MEM_HANDLERS_16BIT_BE(16)
GENERATE_MEM_HANDLERS_16BIT_BE(18)
GENERATE_MEM_HANDLERS_16BIT_BE(24)
GENERATE_MEM_HANDLERS_16BIT_BE(32)

GENERATE_MEM_HANDLERS_16BIT_LE(16)
GENERATE_MEM_HANDLERS_16BIT_LE(17)
GENERATE_MEM_HANDLERS_16BIT_LE(24)
GENERATE_MEM_HANDLERS_16BIT_LE(29)
GENERATE_MEM_HANDLERS_16BIT_LE(32)

GENERATE_MEM_HANDLERS_32BIT_BE(24)
GENERATE_MEM_HANDLERS_32BIT_BE(29)
GENERATE_MEM_HANDLERS_32BIT_BE(32)

GENERATE_MEM_HANDLERS_32BIT_LE(24)
GENERATE_MEM_HANDLERS_32BIT_LE(26)
GENERATE_MEM_HANDLERS_32BIT_LE(29)
GENERATE_MEM_HANDLERS_32BIT_LE(32)

GENERATE_MEM_HANDLERS_32BIT_BE(18)	/* HACK -- used for pdp-1 */

/* make sure you add an entry to this list whenever you add a set of handlers */
static const struct memory_address_table readmem_to_bits[] =
{
	{ 16, cpu_readmem16 },
	{ 20, cpu_readmem20 },
	{ 21, cpu_readmem21 },
	{ 24, cpu_readmem24 },

	{ 16, cpu_readmem16bew },
	{ 18, cpu_readmem18bew },
	{ 24, cpu_readmem24bew },
	{ 32, cpu_readmem32bew },

	{ 16, cpu_readmem16lew },
	{ 17, cpu_readmem17lew },
	{ 24, cpu_readmem24lew },
	{ 29, cpu_readmem29lew },
	{ 32, cpu_readmem32lew },

	{ 24, cpu_readmem24bedw },
	{ 29, cpu_readmem29bedw },
	{ 32, cpu_readmem32bedw },

	{ 24, cpu_readmem24ledw },
	{ 26, cpu_readmem26ledw },
	{ 29, cpu_readmem29ledw },
	{ 32, cpu_readmem32ledw },

	{ 18, cpu_readmem18bedw }
};


/*-------------------------------------------------
	the port handlers we need to generate
-------------------------------------------------*/

GENERATE_PORT_HANDLERS_8BIT(16)

GENERATE_PORT_HANDLERS_16BIT_BE(16)

GENERATE_PORT_HANDLERS_16BIT_LE(16)
GENERATE_PORT_HANDLERS_16BIT_LE(24)

GENERATE_PORT_HANDLERS_32BIT_BE(16)

GENERATE_PORT_HANDLERS_32BIT_LE(16)
GENERATE_PORT_HANDLERS_32BIT_LE(24)
GENERATE_PORT_HANDLERS_32BIT_LE(32)


/*-------------------------------------------------
	safe opcode reading
-------------------------------------------------*/

data8_t cpu_readop_safe(offs_t offset)
{
	activecpu_set_op_base(offset);
	return cpu_readop_unsafe(offset);
}

data16_t cpu_readop16_safe(offs_t offset)
{
	activecpu_set_op_base(offset);
	return cpu_readop16_unsafe(offset);
}

data32_t cpu_readop32_safe(offs_t offset)
{
	activecpu_set_op_base(offset);
	return cpu_readop32_unsafe(offset);
}

data8_t cpu_readop_arg_safe(offs_t offset)
{
	activecpu_set_op_base(offset);
	return cpu_readop_arg_unsafe(offset);
}

data16_t cpu_readop_arg16_safe(offs_t offset)
{
	activecpu_set_op_base(offset);
	return cpu_readop_arg16_unsafe(offset);
}

data32_t cpu_readop_arg32_safe(offs_t offset)
{
	activecpu_set_op_base(offset);
	return cpu_readop_arg32_unsafe(offset);
}


/*-------------------------------------------------
	get address bits from a read handler
-------------------------------------------------*/

int mem_address_bits_of_cpu(int cputype)
{
	read8_handler handler = cputype_get_interface(cputype)->memory_read;
	int	idx;

	/* scan the table */
	for (idx = 0; idx < sizeof(readmem_to_bits) / sizeof(readmem_to_bits[0]); idx++)
		if (readmem_to_bits[idx].handler == handler)
			return readmem_to_bits[idx].bits;

	/* this is a fatal error */
	fatalerror("CPU memory handlers don't have a table entry in readmem_to_bits!\n");
	exit(1);
	return 0;
}


/*-------------------------------------------------
	get address bits from a read handler
-------------------------------------------------*/

int port_address_bits_of_cpu(int cputype)
{
#if (HAS_V60)
	return cputype == CPU_V60 ? 24 : 16;
#else
	return 16;
#endif
}


/*-------------------------------------------------
	basic static handlers
-------------------------------------------------*/

static INLINE offs_t effective_offset(offs_t offset)
{
	int shift = activecpu_address_shift();
	if (shift < 0)
		return offset >> -shift;
	else
		return offset << shift;
}

static READ_HANDLER( mrh8_bad )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): unmapped memory byte read from %08X\n", cpu_getactivecpu(), activecpu_get_pc(), effective_offset(offset));
	if (activecpu_address_bits() <= SPARSE_THRESH && unmap_value == 0) return cpu_bankbase[STATIC_RAM][offset];
	return unmap_value;
}
static READ16_HANDLER( mrh16_bad )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): unmapped memory word read from %08X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), effective_offset(offset*2), mem_mask ^ 0xffff);
	if (activecpu_address_bits() <= SPARSE_THRESH && unmap_value == 0) return ((data16_t *)cpu_bankbase[STATIC_RAM])[offset];
	return unmap_value;
}
static READ32_HANDLER( mrh32_bad )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): unmapped memory dword read from %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), effective_offset(offset*4), mem_mask ^ 0xffffffff);
	if (activecpu_address_bits() <= SPARSE_THRESH && unmap_value == 0) return ((data32_t *)cpu_bankbase[STATIC_RAM])[offset];
	return unmap_value;
}

static WRITE_HANDLER( mwh8_bad )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): unmapped memory byte write to %08X = %02X\n", cpu_getactivecpu(), activecpu_get_pc(), effective_offset(offset), data);
	if (activecpu_address_bits() <= SPARSE_THRESH) cpu_bankbase[STATIC_RAM][offset] = data;
}
static WRITE16_HANDLER( mwh16_bad )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): unmapped memory word write to %08X = %04X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), effective_offset(offset*2), data, mem_mask ^ 0xffff);
	if (activecpu_address_bits() <= SPARSE_THRESH) COMBINE_DATA(&((data16_t *)cpu_bankbase[STATIC_RAM])[offset]);
}
static WRITE32_HANDLER( mwh32_bad )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): unmapped memory dword write to %08X = %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), effective_offset(offset*4), data, mem_mask ^ 0xffffffff);
	if (activecpu_address_bits() <= SPARSE_THRESH) COMBINE_DATA(&((data32_t *)cpu_bankbase[STATIC_RAM])[offset]);
}

static READ_HANDLER( prh8_bad )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): unmapped port byte read from %08X\n", cpu_getactivecpu(), activecpu_get_pc(), offset);
	return unmap_value;
}
static READ16_HANDLER( prh16_bad )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): unmapped port word read from %08X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), offset*2, mem_mask ^ 0xffff);
	return unmap_value;
}
static READ32_HANDLER( prh32_bad )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): unmapped port dword read from %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), offset*4, mem_mask ^ 0xffffffff);
	return unmap_value;
}

static WRITE_HANDLER( pwh8_bad )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): unmapped port byte write to %08X = %02X\n", cpu_getactivecpu(), activecpu_get_pc(), offset, data);
}
static WRITE16_HANDLER( pwh16_bad )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): unmapped port word write to %08X = %04X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), offset*2, data, mem_mask ^ 0xffff);
}
static WRITE32_HANDLER( pwh32_bad )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): unmapped port dword write to %08X = %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), offset*4, data, mem_mask ^ 0xffffffff);
}

static WRITE_HANDLER( mwh8_rom )
{ 
  log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): byte write to ROM %08X = %02X\n", cpu_getactivecpu(), activecpu_get_pc(), effective_offset(offset), data); 
}

static WRITE16_HANDLER( mwh16_rom )
{
  log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): word write to %08X = %04X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), effective_offset(offset*2), data, mem_mask ^ 0xffff);
}

static WRITE32_HANDLER( mwh32_rom )
{
  log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): dword write to %08X = %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), effective_offset(offset*4), data, mem_mask ^ 0xffffffff);
}

static READ_HANDLER( mrh8_nop )        { return 0; }
static READ16_HANDLER( mrh16_nop )     { return 0; }
static READ32_HANDLER( mrh32_nop )     { return 0; }

static WRITE_HANDLER( mwh8_nop )       {  }
static WRITE16_HANDLER( mwh16_nop )    {  }
static WRITE32_HANDLER( mwh32_nop )    {  }

static READ_HANDLER( mrh8_ram )        { return cpu_bankbase[STATIC_RAM][offset]; }
static WRITE_HANDLER( mwh8_ram )       { cpu_bankbase[STATIC_RAM][offset] = data; }

static WRITE_HANDLER( mwh8_ramrom )    { cpu_bankbase[STATIC_RAM][offset] = cpu_bankbase[STATIC_RAM][offset + (OP_ROM - OP_RAM)] = data; }
static WRITE16_HANDLER( mwh16_ramrom ) { COMBINE_DATA(&cpu_bankbase[STATIC_RAM][offset*2]); COMBINE_DATA(&cpu_bankbase[0][offset*2 + (OP_ROM - OP_RAM)]); }
static WRITE32_HANDLER( mwh32_ramrom ) { COMBINE_DATA(&cpu_bankbase[STATIC_RAM][offset*4]); COMBINE_DATA(&cpu_bankbase[0][offset*4 + (OP_ROM - OP_RAM)]); }

static READ_HANDLER( mrh8_bank1 )      { return cpu_bankbase[1][offset]; }
static READ_HANDLER( mrh8_bank2 )      { return cpu_bankbase[2][offset]; }
static READ_HANDLER( mrh8_bank3 )      { return cpu_bankbase[3][offset]; }
static READ_HANDLER( mrh8_bank4 )      { return cpu_bankbase[4][offset]; }
static READ_HANDLER( mrh8_bank5 )      { return cpu_bankbase[5][offset]; }
static READ_HANDLER( mrh8_bank6 )      { return cpu_bankbase[6][offset]; }
static READ_HANDLER( mrh8_bank7 )      { return cpu_bankbase[7][offset]; }
static READ_HANDLER( mrh8_bank8 )      { return cpu_bankbase[8][offset]; }
static READ_HANDLER( mrh8_bank9 )      { return cpu_bankbase[9][offset]; }
static READ_HANDLER( mrh8_bank10 )     { return cpu_bankbase[10][offset]; }
static READ_HANDLER( mrh8_bank11 )     { return cpu_bankbase[11][offset]; }
static READ_HANDLER( mrh8_bank12 )     { return cpu_bankbase[12][offset]; }
static READ_HANDLER( mrh8_bank13 )     { return cpu_bankbase[13][offset]; }
static READ_HANDLER( mrh8_bank14 )     { return cpu_bankbase[14][offset]; }
static READ_HANDLER( mrh8_bank15 )     { return cpu_bankbase[15][offset]; }
static READ_HANDLER( mrh8_bank16 )     { return cpu_bankbase[16][offset]; }
static READ_HANDLER( mrh8_bank17 )     { return cpu_bankbase[17][offset]; }
static READ_HANDLER( mrh8_bank18 )     { return cpu_bankbase[18][offset]; }
static READ_HANDLER( mrh8_bank19 )     { return cpu_bankbase[19][offset]; }
static READ_HANDLER( mrh8_bank20 )     { return cpu_bankbase[20][offset]; }
static READ_HANDLER( mrh8_bank21 )     { return cpu_bankbase[21][offset]; }
static READ_HANDLER( mrh8_bank22 )     { return cpu_bankbase[22][offset]; }
static READ_HANDLER( mrh8_bank23 )     { return cpu_bankbase[23][offset]; }
static READ_HANDLER( mrh8_bank24 )     { return cpu_bankbase[24][offset]; }

static WRITE_HANDLER( mwh8_bank1 )     { cpu_bankbase[1][offset] = data; }
static WRITE_HANDLER( mwh8_bank2 )     { cpu_bankbase[2][offset] = data; }
static WRITE_HANDLER( mwh8_bank3 )     { cpu_bankbase[3][offset] = data; }
static WRITE_HANDLER( mwh8_bank4 )     { cpu_bankbase[4][offset] = data; }
static WRITE_HANDLER( mwh8_bank5 )     { cpu_bankbase[5][offset] = data; }
static WRITE_HANDLER( mwh8_bank6 )     { cpu_bankbase[6][offset] = data; }
static WRITE_HANDLER( mwh8_bank7 )     { cpu_bankbase[7][offset] = data; }
static WRITE_HANDLER( mwh8_bank8 )     { cpu_bankbase[8][offset] = data; }
static WRITE_HANDLER( mwh8_bank9 )     { cpu_bankbase[9][offset] = data; }
static WRITE_HANDLER( mwh8_bank10 )    { cpu_bankbase[10][offset] = data; }
static WRITE_HANDLER( mwh8_bank11 )    { cpu_bankbase[11][offset] = data; }
static WRITE_HANDLER( mwh8_bank12 )    { cpu_bankbase[12][offset] = data; }
static WRITE_HANDLER( mwh8_bank13 )    { cpu_bankbase[13][offset] = data; }
static WRITE_HANDLER( mwh8_bank14 )    { cpu_bankbase[14][offset] = data; }
static WRITE_HANDLER( mwh8_bank15 )    { cpu_bankbase[15][offset] = data; }
static WRITE_HANDLER( mwh8_bank16 )    { cpu_bankbase[16][offset] = data; }
static WRITE_HANDLER( mwh8_bank17 )    { cpu_bankbase[17][offset] = data; }
static WRITE_HANDLER( mwh8_bank18 )    { cpu_bankbase[18][offset] = data; }
static WRITE_HANDLER( mwh8_bank19 )    { cpu_bankbase[19][offset] = data; }
static WRITE_HANDLER( mwh8_bank20 )    { cpu_bankbase[20][offset] = data; }
static WRITE_HANDLER( mwh8_bank21 )    { cpu_bankbase[21][offset] = data; }
static WRITE_HANDLER( mwh8_bank22 )    { cpu_bankbase[22][offset] = data; }
static WRITE_HANDLER( mwh8_bank23 )    { cpu_bankbase[23][offset] = data; }
static WRITE_HANDLER( mwh8_bank24 )    { cpu_bankbase[24][offset] = data; }


/*-------------------------------------------------
	init_static - sets up the static memory
	handlers
-------------------------------------------------*/

static int init_static(void)
{
	memset(rmemhandler8,  0, sizeof(rmemhandler8));
	memset(rmemhandler8s, 0, sizeof(rmemhandler8s));
	memset(rmemhandler16, 0, sizeof(rmemhandler16));
	memset(rmemhandler32, 0, sizeof(rmemhandler32));
	memset(wmemhandler8,  0, sizeof(wmemhandler8));
	memset(wmemhandler8s, 0, sizeof(wmemhandler8s));
	memset(wmemhandler16, 0, sizeof(wmemhandler16));
	memset(wmemhandler32, 0, sizeof(wmemhandler32));

	memset(rporthandler8,  0, sizeof(rporthandler8));
	memset(rporthandler16, 0, sizeof(rporthandler16));
	memset(rporthandler32, 0, sizeof(rporthandler32));
	memset(wporthandler8,  0, sizeof(wporthandler8));
	memset(wporthandler16, 0, sizeof(wporthandler16));
	memset(wporthandler32, 0, sizeof(wporthandler32));

	set_static_handler(STATIC_BANK1,  mrh8_bank1,  NULL,         NULL,         mwh8_bank1,  NULL,         NULL);
	set_static_handler(STATIC_BANK2,  mrh8_bank2,  NULL,         NULL,         mwh8_bank2,  NULL,         NULL);
	set_static_handler(STATIC_BANK3,  mrh8_bank3,  NULL,         NULL,         mwh8_bank3,  NULL,         NULL);
	set_static_handler(STATIC_BANK4,  mrh8_bank4,  NULL,         NULL,         mwh8_bank4,  NULL,         NULL);
	set_static_handler(STATIC_BANK5,  mrh8_bank5,  NULL,         NULL,         mwh8_bank5,  NULL,         NULL);
	set_static_handler(STATIC_BANK6,  mrh8_bank6,  NULL,         NULL,         mwh8_bank6,  NULL,         NULL);
	set_static_handler(STATIC_BANK7,  mrh8_bank7,  NULL,         NULL,         mwh8_bank7,  NULL,         NULL);
	set_static_handler(STATIC_BANK8,  mrh8_bank8,  NULL,         NULL,         mwh8_bank8,  NULL,         NULL);
	set_static_handler(STATIC_BANK9,  mrh8_bank9,  NULL,         NULL,         mwh8_bank9,  NULL,         NULL);
	set_static_handler(STATIC_BANK10, mrh8_bank10, NULL,         NULL,         mwh8_bank10, NULL,         NULL);
	set_static_handler(STATIC_BANK11, mrh8_bank11, NULL,         NULL,         mwh8_bank11, NULL,         NULL);
	set_static_handler(STATIC_BANK12, mrh8_bank12, NULL,         NULL,         mwh8_bank12, NULL,         NULL);
	set_static_handler(STATIC_BANK13, mrh8_bank13, NULL,         NULL,         mwh8_bank13, NULL,         NULL);
	set_static_handler(STATIC_BANK14, mrh8_bank14, NULL,         NULL,         mwh8_bank14, NULL,         NULL);
	set_static_handler(STATIC_BANK15, mrh8_bank15, NULL,         NULL,         mwh8_bank15, NULL,         NULL);
	set_static_handler(STATIC_BANK16, mrh8_bank16, NULL,         NULL,         mwh8_bank16, NULL,         NULL);
	set_static_handler(STATIC_BANK17, mrh8_bank17, NULL,         NULL,         mwh8_bank17, NULL,         NULL);
	set_static_handler(STATIC_BANK18, mrh8_bank18, NULL,         NULL,         mwh8_bank18, NULL,         NULL);
	set_static_handler(STATIC_BANK19, mrh8_bank19, NULL,         NULL,         mwh8_bank19, NULL,         NULL);
	set_static_handler(STATIC_BANK20, mrh8_bank20, NULL,         NULL,         mwh8_bank20, NULL,         NULL);
	set_static_handler(STATIC_BANK21, mrh8_bank21, NULL,         NULL,         mwh8_bank21, NULL,         NULL);
	set_static_handler(STATIC_BANK22, mrh8_bank22, NULL,         NULL,         mwh8_bank22, NULL,         NULL);
	set_static_handler(STATIC_BANK23, mrh8_bank23, NULL,         NULL,         mwh8_bank23, NULL,         NULL);
	set_static_handler(STATIC_BANK24, mrh8_bank24, NULL,         NULL,         mwh8_bank24, NULL,         NULL);
	set_static_handler(STATIC_UNMAP,  mrh8_bad,    mrh16_bad,    mrh32_bad,    mwh8_bad,    mwh16_bad,    mwh32_bad);
	set_static_handler(STATIC_NOP,    mrh8_nop,    mrh16_nop,    mrh32_nop,    mwh8_nop,    mwh16_nop,    mwh32_nop);
	set_static_handler(STATIC_RAM,    mrh8_ram,    NULL,         NULL,         mwh8_ram,    NULL,         NULL);
	set_static_handler(STATIC_ROM,    NULL,        NULL,         NULL,         mwh8_rom,    mwh16_rom,    mwh32_rom);
	set_static_handler(STATIC_RAMROM, NULL,        NULL,         NULL,         mwh8_ramrom, mwh16_ramrom, mwh32_ramrom);

	/* override port unmapped handlers */
	rporthandler8 [STATIC_UNMAP].handler = (void *)prh8_bad;
	rporthandler16[STATIC_UNMAP].handler = (void *)prh16_bad;
	rporthandler32[STATIC_UNMAP].handler = (void *)prh32_bad;
	wporthandler8 [STATIC_UNMAP].handler = (void *)pwh8_bad;
	wporthandler16[STATIC_UNMAP].handler = (void *)pwh16_bad;
	wporthandler32[STATIC_UNMAP].handler = (void *)pwh32_bad;

	return 1;
}


/*-------------------------------------------------
	debugging
-------------------------------------------------*/

#ifdef MEM_DUMP

static void dump_map(FILE *file, const struct memport_data *memport, const struct table_data *table)
{
	static const char *strings[] =
	{
		"invalid",		"bank 1",		"bank 2",		"bank 3",
		"bank 4",		"bank 5",		"bank 6",		"bank 7",
		"bank 8",		"bank 9",		"bank 10",		"bank 11",
		"bank 12",		"bank 13",		"bank 14",		"bank 15",
		"bank 16",		"bank 17",		"bank 18",		"bank 19",
		"bank 20",		"bank 21",		"bank 22",		"bank 23",
		"bank 24",		"RAM",			"ROM",			"RAMROM",
		"nop",			"unused 1",		"unused 2",		"unmapped"
	};

	int minbits = DATABITS_TO_SHIFT(memport->dbits);
	int l1bits = LEVEL1_BITS(memport->ebits);
	int l2bits = LEVEL2_BITS(memport->ebits);
	int l1count = 1 << l1bits;
	int l2count = 1 << l2bits;
	int i, j;

	fprintf(file, "  Address bits = %d\n", memport->abits);
	fprintf(file, "     Data bits = %d\n", memport->dbits);
	fprintf(file, "Effective bits = %d\n", memport->ebits);
	fprintf(file, "       L1 bits = %d\n", l1bits);
	fprintf(file, "       L2 bits = %d\n", l2bits);
	fprintf(file, "  Address mask = %X\n", memport->mask);
	fprintf(file, "\n");

	for (i = 0; i < l1count; i++)
	{
		UINT8 entry = table->table[i];
		if (entry != STATIC_UNMAP)
		{
			fprintf(file, "%05X  %08X-%08X    = %02X: ", i,
					i << (l2bits + minbits),
					((i+1) << (l2bits + minbits)) - 1, entry);
			if (entry < STATIC_COUNT)
				fprintf(file, "%s [offset=%08X]\n", strings[entry], table->handlers[entry].offset);
			else if (entry < SUBTABLE_BASE)
				fprintf(file, "handler(%08X) [offset=%08X]\n", (UINT32)table->handlers[entry].handler, table->handlers[entry].offset);
			else
			{
				fprintf(file, "subtable %d\n", entry & SUBTABLE_MASK);
				entry &= SUBTABLE_MASK;

				for (j = 0; j < l2count; j++)
				{
					UINT8 entry2 = table->table[(1 << l1bits) + (entry << l2bits) + j];
					if (entry2 != STATIC_UNMAP)
					{
						fprintf(file, "   %05X  %08X-%08X = %02X: ", j,
								(i << (l2bits + minbits)) | (j << minbits),
								((i << (l2bits + minbits)) | ((j+1) << minbits)) - 1, entry2);
						if (entry2 < STATIC_COUNT)
							fprintf(file, "%s [offset=%08X]\n", strings[entry2], table->handlers[entry2].offset);
						else if (entry2 < SUBTABLE_BASE)
							fprintf(file, "handler(%08X) [offset=%08X]\n", (UINT32)table->handlers[entry2].handler, table->handlers[entry2].offset);
						else
							fprintf(file, "subtable %d???????????\n", entry2 & SUBTABLE_MASK);
					}
				}
			}
		}
	}
}

static void mem_dump(void)
{
	FILE *file = fopen("memdump.log", "w");
	int cpunum;

	/* skip if we can't open the file */
	if (!file)
		return;

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		/* memory handlers */
		if (cpudata[cpunum].mem.abits)
		{
			fprintf(file, "\n\n"
			              "===============================\n"
			              "CPU %d read memory handler dump\n"
			              "===============================\n", cpunum);
			dump_map(file, &cpudata[cpunum].mem, &cpudata[cpunum].mem.read);

			fprintf(file, "\n\n"
			              "================================\n"
			              "CPU %d write memory handler dump\n"
			              "================================\n", cpunum);
			dump_map(file, &cpudata[cpunum].mem, &cpudata[cpunum].mem.write);
		}

		/* port handlers */
		if (cpudata[cpunum].port.abits)
		{
			fprintf(file, "\n\n"
			              "=============================\n"
			              "CPU %d read port handler dump\n"
			              "=============================\n", cpunum);
			dump_map(file, &cpudata[cpunum].port, &cpudata[cpunum].port.read);

			fprintf(file, "\n\n"
			              "==============================\n"
			              "CPU %d write port handler dump\n"
			              "==============================\n", cpunum);
			dump_map(file, &cpudata[cpunum].port, &cpudata[cpunum].port.write);
		}
	}
	fclose(file);
}
#endif

#ifdef CHECK_MASKS
static void *track_buffer[65536];
static int track_count;
static int static_count;
static int track_entry(void *entry, void *handler)
{
	int i;
	for (i = 0; i < track_count; i++)
		if (track_buffer[i] == entry)
			return 1;
	track_buffer[track_count++] = entry;
	if (HANDLER_IS_STATIC(handler))
		static_count++;
	return 0;
}

static void verify_masks(void)
{
	int i, cpunum;

	for (i = 0;drivers[i];i++)
	{
		const struct RomModule *romp = drivers[i]->rom;
		if (romp)
		{
			for (cpunum = 0;cpunum < MAX_CPU;cpunum++)
			{
				if (drivers[i]->drv->cpu[cpunum].cpu_type)
				{
					const struct Memory_ReadAddress *mra = drivers[i]->drv->cpu[cpunum].memory_read;
					const struct Memory_WriteAddress *mwa = drivers[i]->drv->cpu[cpunum].memory_write;
					const struct IO_ReadPort *iora = drivers[i]->drv->cpu[cpunum].port_read;
					const struct IO_WritePort *iowa = drivers[i]->drv->cpu[cpunum].port_write;

					if (mra)
						for ( ; !IS_MEMPORT_END(mra); mra++)
							if (!IS_MEMPORT_MARKER(mra))
							{
								size_t size = mra->end - mra->start + 1;
								if (size)
								{
									while (!(size & 1)) size >>= 1;
									if (size != 1)
									{
										if (!track_entry((void *)mra, (void *)mra->handler))
											log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: %s cpu %d readmem inval size  { %08X, %08X }\n", drivers[i]->source_file, drivers[i]->name, cpunum, mra->start, mra->end);
									}
								}
								size--;
								if ((mra->start & size) != 0)
								{
									if (!track_entry((void *)mra, (void *)mra->handler))
										log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: %s cpu %d readmem inval start { %08X, %08X }\n", drivers[i]->source_file, drivers[i]->name, cpunum, mra->start, mra->end);
								}
								if ((mra->end & size) != size)
								{
									if (!track_entry((void *)mra, (void *)mra->handler))
										log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: %s cpu %d readmem inval end  { %08X, %08X }\n", drivers[i]->source_file, drivers[i]->name, cpunum, mra->start, mra->end);
								}
							}

					if (mwa)
						for ( ; !IS_MEMPORT_END(mwa); mwa++)
							if (!IS_MEMPORT_MARKER(mwa))
							{
								size_t size = mwa->end - mwa->start + 1;
								if (size)
								{
									while (!(size & 1)) size >>= 1;
									if (size != 1)
									{
										if (!track_entry((void *)mwa, (void *)mwa->handler))
											log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: %s cpu %d writemem inval size  { %08X, %08X }\n", drivers[i]->source_file, drivers[i]->name, cpunum, mwa->start, mwa->end);
									}
								}
								size--;
								if ((mwa->start & size) != 0)
								{
									if (!track_entry((void *)mwa, (void *)mwa->handler))
										log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: %s cpu %d writemem inval start { %08X, %08X }\n", drivers[i]->source_file, drivers[i]->name, cpunum, mwa->start, mwa->end);
								}
								if ((mwa->end & size) != size)
								{
									if (!track_entry((void *)mwa, (void *)mwa->handler))
										log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: %s cpu %d writemem inval end  { %08X, %08X }\n", drivers[i]->source_file, drivers[i]->name, cpunum, mwa->start, mwa->end);
								}
							}

					if (iora)
						for ( ; !IS_MEMPORT_END(iora); iora++)
							if (!IS_MEMPORT_MARKER(iora))
							{
								size_t size = iora->end - iora->start + 1;
								if (size)
								{
									while (!(size & 1)) size >>= 1;
									if (size != 1)
									{
										if (!track_entry((void *)iora, (void *)iora->handler))
											log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: %s cpu %d readmem inval size  { %08X, %08X }\n", drivers[i]->source_file, drivers[i]->name, cpunum, iora->start, iora->end);
									}
								}
								size--;
								if ((iora->start & size) != 0)
								{
									if (!track_entry((void *)iora, (void *)iora->handler))
										log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: %s cpu %d readmem inval start { %08X, %08X }\n", drivers[i]->source_file, drivers[i]->name, cpunum, iora->start, iora->end);
								}
								if ((iora->end & size) != size)
								{
									if (!track_entry((void *)iora, (void *)iora->handler))
										log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: %s cpu %d readmem inval end  { %08X, %08X }\n", drivers[i]->source_file, drivers[i]->name, cpunum, iora->start, iora->end);
								}
							}

					if (iowa)
						for ( ; !IS_MEMPORT_END(iowa); iowa++)
							if (!IS_MEMPORT_MARKER(iowa))
							{
								size_t size = iowa->end - iowa->start + 1;
								if (size)
								{
									while (!(size & 1)) size >>= 1;
									if (size != 1)
									{
										if (!track_entry((void *)iowa, (void *)iowa->handler))
											log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: %s cpu %d writemem inval size  { %08X, %08X }\n", drivers[i]->source_file, drivers[i]->name, cpunum, iowa->start, iowa->end);
									}
								}
								size--;
								if ((iowa->start & size) != 0)
								{
									if (!track_entry((void *)iowa, (void *)iowa->handler))
										log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: %s cpu %d writemem inval start { %08X, %08X }\n", drivers[i]->source_file, drivers[i]->name, cpunum, iowa->start, iowa->end);
								}
								if ((iowa->end & size) != size)
								{
									if (!track_entry((void *)iowa, (void *)iowa->handler))
										log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: %s cpu %d writemem inval end  { %08X, %08X }\n", drivers[i]->source_file, drivers[i]->name, cpunum, iowa->start, iowa->end);
								}
							}
				}
			}
		}
	}
	log_cb(RETRO_LOG_INFO, LOGPRE "mame-memory: Total busted entries = %d\n", track_count);
	log_cb(RETRO_LOG_INFO, LOGPRE "mame-memory: Busted entries that are static = %d\n", static_count);
}
#endif
