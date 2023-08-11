#include "v60.h"

void v60Init();
void v70Init();

void v60Exit();
void v60Reset();
void v60Open(int cpu);
void v60Close();

INT32 v60Run(int cycles);

void v60SetIRQLine(INT32 irqline, INT32 state);

void v60SetIRQCallback(int (*callback)(int irqline));
void v60SetWriteByteHandler(void (*write)(UINT32,UINT8));
void v60SetWriteWordHandler(void (*write)(UINT32,UINT16));
void v60SetWriteLongHandler(void (*write)(UINT32,UINT32));
void v60SetReadByteHandler(UINT8  (*read)(UINT32));
void v60SetReadWordHandler(UINT16 (*read)(UINT32));
void v60SetReadLongHandler(UINT32 (*read)(UINT32));
void v60SetAddressMask(UINT32 mask);
void v60MapMemory(UINT8 *ptr, UINT64 start, UINT64 end, UINT32 flags);

void v60WriteWord(UINT32 address, UINT16 data);
void v60WriteByte(UINT32 address, UINT8 data);
UINT16 v60ReadWord(UINT32 address);
UINT16 v60ReadByte(UINT32 address);

UINT32 v60GetPC(INT32);
UINT32 v60GetRegs(INT32 regs);

INT32 v60TotalCycles();
void v60RunEnd();
void v60NewFrame();
INT32 v60GetActive();
INT32 v60Idle(INT32 cycles);

INT32 v60Scan(INT32 nAction);

void v60WriteROM(UINT32 a, UINT8 d);
UINT8 v60CheatRead(UINT32 a);

extern struct cpu_core_config v60Config;

// depreciate this and use BurnTimerAttach directly!
#define BurnTimerAttachV60(clock)	\
	BurnTimerAttach(&v60Config, clock)
