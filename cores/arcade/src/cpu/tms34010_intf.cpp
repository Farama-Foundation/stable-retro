#include "burnint.h"
#include "tms34010/tms34010.h"
#include "tms34010_intf.h"

#define ADDR_BITS   32
#define PAGE_SIZE   0x1000
#define PAGE_SIZE_8 (0x1000 >> 3)
#define PAGE_SHIFT  12
#define PAGE_MASK   0xFFF
#define PAGE_COUNT  (1 << (ADDR_BITS - PAGE_SHIFT))
#define PAGE_WADD    (PAGE_COUNT)
#define MAXHANDLER  32
#define PFN(x)  (((x) >> PAGE_SHIFT) & 0xFFFFF)

template<typename T>
inline T tms_fast_read(UINT8 *ptr, UINT32 adr) {
    return *((T*)  ((UINT8*) ptr + TOBYTE(adr & PAGE_MASK)));
}

template<typename T>
inline void tms_fast_write(UINT8 *xptr, UINT32 adr, T value) {
    T *ptr = ((T*)  ((UINT8*) xptr + TOBYTE(adr & PAGE_MASK)));
    *ptr = value;
}

static TMS34010State tms34010;
static pTMS34010ScanlineRender scanlineRenderCallback = NULL;

struct TMS34010MemoryMap
{
    UINT8 *map[PAGE_COUNT * 2];

    pTMS34010ReadHandler read[MAXHANDLER];
    pTMS34010WriteHandler write[MAXHANDLER];
};

static TMS34010MemoryMap g_mmap;

static UINT16 default_read(UINT32 address) { return ~0; }
static void default_write(UINT32 address, UINT16 value) {}
static void default_shift_op(UINT32,void*){}


static UINT16 IO_read(UINT32 address) { return tms::read_ioreg(&tms34010,address); }
static void IO_write(UINT32 address, UINT16 value) { tms::write_ioreg(&tms34010, address, value); }

// cheat-engine hook-up
void TMS34010Open(INT32 num)
{
	// not used, single core.
}

void TMS34010Close()
{
	// not used, single core.
}

INT32 TMS34010GetActive()
{
	return 0; // cpu is always active
}

int TMS34010Idle(int cycles)
{
    return 0; // not impl.
}

void TMS34010WriteROM(UINT32 address, UINT8 value);
UINT8 TMS34010ReadByte(UINT32 address);

INT32 TMS34010TotalCyclesi32()
{
	return TMS34010TotalCycles();
}

void TMS34010SetIRQLine(INT32 cpu, INT32 vector, INT32 status)
{
	if (status == CPU_IRQSTATUS_NONE) {
		TMS34010ClearIRQ(vector);
	}

	if (status == CPU_IRQSTATUS_ACK) {
		TMS34010GenerateIRQ(vector);
	}
}

cpu_core_config TMS34010Config =
{
	"TMS34010",
	TMS34010Open,
	TMS34010Close,
	TMS34010ReadByte,
	TMS34010WriteROM,
	TMS34010GetActive,
	TMS34010TotalCyclesi32,
	TMS34010NewFrame,
	TMS34010Idle,
	TMS34010SetIRQLine,
	TMS34010Run,
	TMS34010RunEnd,
	TMS34010Reset,
	0x100000000ULL,
	0
};
// end cheat-engine hook-up

void TMS34010Init()
{
    tms34010.shift_read_cycle = default_shift_op;
    tms34010.shift_write_cycle = default_shift_op;

    // map IO registers
    TMS34010SetHandlers(MAXHANDLER-1, IO_read, IO_write);
	TMS34010MapHandler(MAXHANDLER-1, 0xc0000000, 0xc00001ff, MAP_READ | MAP_WRITE);

	CpuCheatRegister(0, &TMS34010Config);
}

int TMS34010Run(int cycles)
{
    return tms::run(&tms34010, cycles);
}

void TMS34010TimerSetCB(void (*timer_cb)())
{
	tms::timer_set_cb(&tms34010, timer_cb);
}

void TMS34010TimerSet(int cycles)
{
	tms::timer_arm(&tms34010, cycles);
}

INT64 TMS34010TotalCycles()
{
	return tms::total_cycles(&tms34010);
}

void TMS34010NewFrame()
{
	tms::new_frame(&tms34010);
}

void TMS34010RunEnd()
{
	tms::stop(&tms34010);
}

void TMS34010Scan(INT32 nAction)
{
	tms::scan(&tms34010, nAction);
}

UINT32 TMS34010GetPC()
{
	return tms::get_pc(&tms34010);
}

UINT32 TMS34010GetPPC()
{
	return tms::get_ppc(&tms34010);
}

void TMS34010Reset()
{
    tms::reset(&tms34010);
}

void TMS34010GenerateIRQ(UINT32 line)
{
    tms::generate_irq(&tms34010, line);
}

void TMS34010ClearIRQ(UINT32 line)
{
    tms::clear_irq(&tms34010, line);
}

void TMS34010SetScanlineRender(pTMS34010ScanlineRender sr)
{
    scanlineRenderCallback = sr;
}

void TMS34010SetToShift(void (*reader)(UINT32 addr, void *dst))
{
    tms34010.shift_read_cycle = reader;
}

void TMS34010SetFromShift(void (*writer)(UINT32 addr, void *src))
{
    tms34010.shift_write_cycle = writer;
}

int TMS34010GenerateScanline(int line)
{
    return tms::generate_scanline(&tms34010, line, scanlineRenderCallback);
}

TMS34010State *TMS34010GetState()
{
    return &tms34010;
}

UINT8 TMS34010ReadByte(UINT32 address)
{
    UINT8 *pr = g_mmap.map[PFN(address)];
    if ((uintptr_t)pr >= MAXHANDLER) {
        // address is bit-address
        return tms_fast_read<UINT8>(pr,address);
    } else {
        return g_mmap.read[(uintptr_t)pr](address);
    }
}

UINT16 TMS34010ReadWord(UINT32 address)
{
    UINT8 *pr = g_mmap.map[PFN(address)];
    if ((uintptr_t)pr >= MAXHANDLER) {
        // address is bit-address
        return BURN_ENDIAN_SWAP_INT16(tms_fast_read<UINT16>(pr,address));
    } else {
        return g_mmap.read[(uintptr_t)pr](address);
    }
}

void TMS34010WriteWord(UINT32 address, UINT16 value)
{
    UINT8 *pr = g_mmap.map[PAGE_WADD + PFN(address)];
    if ((uintptr_t)pr >= MAXHANDLER) {
        // address is bit-address
        return tms_fast_write<UINT16>(pr,address, BURN_ENDIAN_SWAP_INT16(value));
    } else {
        return g_mmap.write[(uintptr_t)pr](address, value);
    }
}

void TMS34010WriteROM(UINT32 address, UINT8 value) // for cheat-engine
{
    UINT8 *pr = g_mmap.map[PAGE_WADD + PFN(address)];
    if ((uintptr_t)pr >= MAXHANDLER) {
        // address is bit-address
		tms_fast_write<UINT8>(pr,address, value);
    } else {
		g_mmap.write[(uintptr_t)pr](address, value);
	}

	pr = g_mmap.map[PFN(address)];
    if ((uintptr_t)pr >= MAXHANDLER) {
        // address is bit-address
		tms_fast_write<UINT8>(pr,address, value);
	}
}

void TMS34010MapReset()
{
    for (int page = 0; page < PAGE_COUNT; page++) {
        g_mmap.map[page] = NULL;
        g_mmap.map[page + PAGE_WADD] = NULL;
    }
    for (int handler = 0; handler < MAXHANDLER; handler++) {
        g_mmap.read[handler] = default_read;
        g_mmap.write[handler] = default_write;
    }
}

void TMS34010MapMemory(UINT8 *mem, UINT32 start, UINT32 end, UINT8 type)
{
    const int max_pages = (PFN(end) - PFN(start)) + 1;

    int page = PFN(start);
    for (int i = 0; i < max_pages; i++, page++) {

        if (type & MAP_READ)
            g_mmap.map[page] = mem + (PAGE_SIZE_8 * i);

        if (type & MAP_WRITE)
            g_mmap.map[page + PAGE_WADD] = mem + (PAGE_SIZE_8 * i);
    }
}

void TMS34010MapHandler(uintptr_t num, UINT32 start, UINT32 end, UINT8 type)
{
    const int max_pages = (PFN(end) - PFN(start)) + 1;

    int page = PFN(start);
    for (int i = 0; i < max_pages; i++, page++) {

        if (type & MAP_READ)
            g_mmap.map[page] = (UINT8*) num;

        if (type & MAP_WRITE)
            g_mmap.map[page + PAGE_WADD] = (UINT8*) num;
    }
}

int TMS34010SetReadHandler(UINT32 num, pTMS34010ReadHandler handler)
{
    if (num >= MAXHANDLER)
        return 1;
    g_mmap.read[num] = handler;
    return 0;
}

int TMS34010SetWriteHandler(UINT32 num, pTMS34010WriteHandler handler)
{
    if (num >= MAXHANDLER)
        return 1;
    g_mmap.write[num] = handler;
    return 0;
}

int TMS34010SetHandlers(UINT32 num, pTMS34010ReadHandler rhandler, pTMS34010WriteHandler whandler)
{
    if (num >= MAXHANDLER)
        return 1;
    g_mmap.read[num] = rhandler;
    g_mmap.write[num] = whandler;
    return 0;
}

