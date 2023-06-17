/****************************************************************/
/* Structure defining all callbacks for different architectures */
/****************************************************************/

struct cpu_info {
	UINT8  (*mr8) (offs_t address);
	void   (*mw8) (offs_t address, UINT8  data);
	UINT16 (*mr16)(offs_t address);
	void   (*mw16)(offs_t address, UINT16 data);
	UINT32 (*mr32)(offs_t address);
	void   (*mw32)(offs_t address, UINT32 data);
	UINT8  (*pr8) (offs_t address);
	void   (*pw8) (offs_t address, UINT8  data);
	UINT16 (*pr16)(offs_t address);
	void   (*pw16)(offs_t address, UINT16 data);
	UINT32 (*pr32)(offs_t address);
	void   (*pw32)(offs_t address, UINT32 data);
	UINT8  (*or8) (offs_t address);
	UINT16 (*or16)(offs_t address);
	UINT32 (*or32)(offs_t address);
	void   (*chpc)(offs_t newpc);
	UINT32 start_pc;
};



/*****************************************************************/
/* Memory accesses for 16-bit data bus, 24-bit address bus (V60) */
/*****************************************************************/

#define MemRead8_16		cpu_readmem24lew
#define MemWrite8_16	cpu_writemem24lew

static UINT16 MemRead16_16(offs_t address)
{
	if (!(address & 1))
		return cpu_readmem24lew_word(address);
	else
	{
		UINT16 result = cpu_readmem24lew(address);
		return result | cpu_readmem24lew(address + 1) << 8;
	}
}

static void MemWrite16_16(offs_t address, UINT16 data)
{
	if (!(address & 1))
		cpu_writemem24lew_word(address, data);
	else
	{
		cpu_writemem24lew(address, data);
		cpu_writemem24lew(address + 1, data >> 8);
	}
}

static UINT32 MemRead32_16(offs_t address)
{
	if (!(address & 1))
	{
		UINT32 result = cpu_readmem24lew_word(address);
		return result | (cpu_readmem24lew_word(address + 2) << 16);
	}
	else
	{
		UINT32 result = cpu_readmem24lew(address);
		result |= cpu_readmem24lew_word(address + 1) << 8;
		return result | cpu_readmem24lew(address + 3) << 24;
	}
}

static void MemWrite32_16(offs_t address, UINT32 data)
{
	if (!(address & 1))
	{
		cpu_writemem24lew_word(address, data);
		cpu_writemem24lew_word(address + 2, data >> 16);
	}
	else
	{
		cpu_writemem24lew(address, data);
		cpu_writemem24lew_word(address + 1, data >> 8);
		cpu_writemem24lew(address + 3, data >> 24);
	}
}


/***************************************************************/
/* Port accesses for 16-bit data bus, 24-bit address bus (V60) */
/***************************************************************/

#define PortRead8_16		cpu_readport24lew
#define PortWrite8_16		cpu_writeport24lew

static UINT16 PortRead16_16(offs_t address)
{
	if (!(address & 1))
		return cpu_readport24lew_word(address);
	else
	{
		UINT16 result = cpu_readport24lew(address);
		return result | cpu_readport24lew(address + 1) << 8;
	}
}

static void PortWrite16_16(offs_t address, UINT16 data)
{
	if (!(address & 1))
		cpu_writeport24lew_word(address, data);
	else
	{
		cpu_writeport24lew(address, data);
		cpu_writeport24lew(address + 1, data >> 8);
	}
}

static UINT32 PortRead32_16(offs_t address)
{
	if (!(address & 1))
	{
		UINT32 result = cpu_readport24lew_word(address);
		return result | (cpu_readport24lew_word(address + 2) << 16);
	}
	else
	{
		UINT32 result = cpu_readport24lew(address);
		result |= cpu_readport24lew_word(address + 1) << 8;
		return result | cpu_readport24lew(address + 3) << 24;
	}
}

static void PortWrite32_16(offs_t address, UINT32 data)
{
	if (!(address & 1))
	{
		cpu_writeport24lew_word(address, data);
		cpu_writeport24lew_word(address + 2, data >> 16);
	}
	else
	{
		cpu_writeport24lew(address, data);
		cpu_writeport24lew_word(address + 1, data >> 8);
		cpu_writeport24lew(address + 3, data >> 24);
	}
}



/*****************************************************************/
/* Opcode accesses for 16-bit data bus, 24-bit address bus (V60) */
/*****************************************************************/

static UINT8 OpRead8_16(offs_t address)
{
	return OP_ROM[BYTE_XOR_LE(address)];
}

static UINT16 OpRead16_16(offs_t address)
{
	return OP_ROM[BYTE_XOR_LE(address)] | (OP_ROM[BYTE_XOR_LE(address+1)] << 8);
}

static UINT32 OpRead32_16(offs_t address)
{
	return OP_ROM[BYTE_XOR_LE(address)] | (OP_ROM[BYTE_XOR_LE(address+1)] << 8) |
			(OP_ROM[BYTE_XOR_LE(address+2)] << 16) | (OP_ROM[BYTE_XOR_LE(address+3)] << 24);
}

static void ChangePC_16(offs_t pc)
{
	change_pc24lew(pc);
}



/*****************************************************************/
/* Memory accesses for 32-bit data bus, 32-bit address bus (V70) */
/*****************************************************************/

#define MemRead8_32		cpu_readmem32ledw
#define MemWrite8_32	cpu_writemem32ledw

static UINT16 MemRead16_32(offs_t address)
{
	if (!(address & 1))
		return cpu_readmem32ledw_word(address);
	else
	{
		UINT16 result = cpu_readmem32ledw(address);
		return result | cpu_readmem32ledw(address + 1) << 8;
	}
}

static void MemWrite16_32(offs_t address, UINT16 data)
{
	if (!(address & 1))
		cpu_writemem32ledw_word(address, data);
	else
	{
		cpu_writemem32ledw(address, data);
		cpu_writemem32ledw(address + 1, data >> 8);
	}
}

static UINT32 MemRead32_32(offs_t address)
{
	if (!(address & 3))
		return cpu_readmem32ledw_dword(address);
	else if (!(address & 1))
	{
		UINT32 result = cpu_readmem32ledw_word(address);
		return result | (cpu_readmem32ledw_word(address + 2) << 16);
	}
	else
	{
		UINT32 result = cpu_readmem32ledw(address);
		result |= cpu_readmem32ledw_word(address + 1) << 8;
		return result | cpu_readmem32ledw(address + 3) << 24;
	}
}

static void MemWrite32_32(offs_t address, UINT32 data)
{
	if (!(address & 3))
		cpu_writemem32ledw_dword(address, data);
	else if (!(address & 1))
	{
		cpu_writemem32ledw_word(address, data);
		cpu_writemem32ledw_word(address + 2, data >> 16);
	}
	else
	{
		cpu_writemem32ledw(address, data);
		cpu_writemem32ledw_word(address + 1, data >> 8);
		cpu_writemem32ledw(address + 3, data >> 24);
	}
}



/***************************************************************/
/* Port accesses for 32-bit data bus, 32-bit address bus (V70) */
/***************************************************************/

#define PortRead8_32		cpu_readport32ledw
#define PortWrite8_32		cpu_writeport32ledw

static UINT16 PortRead16_32(offs_t address)
{
	if (!(address & 1))
	{
		return cpu_readport32ledw_word(address);
	}
	else
	{
		UINT16 result = cpu_readport32ledw(address);
		return result | cpu_readport32ledw(address + 1) << 8;
	}
}

static void PortWrite16_32(offs_t address, UINT16 data)
{
	if (!(address & 1))
	{
		cpu_writeport32ledw_word(address, data);
	}
	else
	{
		cpu_writeport32ledw(address, data);
		cpu_writeport32ledw(address + 1, data >> 8);
	}
}

static UINT32 PortRead32_32(offs_t address)
{
	if (!(address & 3))
		return cpu_readport32ledw_dword(address);
	else if (!(address & 1))
	{
		UINT32 result = cpu_readport32ledw_word(address);
		return result | (cpu_readport32ledw_word(address + 2) << 16);
	}
	else
	{
		UINT32 result = cpu_readport32ledw(address);
		result |= cpu_readport32ledw_word(address + 1) << 8;
		return result | cpu_readport32ledw(address + 3) << 24;
	}
}

static void PortWrite32_32(offs_t address, UINT32 data)
{
	if (!(address & 3))
		cpu_writeport32ledw_dword(address, data);
	else if (!(address & 1))
	{
		cpu_writeport32ledw_word(address, data);
		cpu_writeport32ledw_word(address + 2, data >> 16);
	}
	else
	{
		cpu_writeport32ledw(address, data);
		cpu_writeport32ledw_word(address + 1, data >> 8);
		cpu_writeport32ledw(address + 3, data >> 24);
	}
}



/*****************************************************************/
/* Opcode accesses for 32-bit data bus, 32-bit address bus (V60) */
/*****************************************************************/

static UINT8 OpRead8_32(offs_t address)
{
	return OP_ROM[BYTE4_XOR_LE(address)];
}

static UINT16 OpRead16_32(offs_t address)
{
	return OP_ROM[BYTE4_XOR_LE(address)] | (OP_ROM[BYTE4_XOR_LE(address+1)] << 8);
}

static UINT32 OpRead32_32(offs_t address)
{
	return OP_ROM[BYTE4_XOR_LE(address)] | (OP_ROM[BYTE4_XOR_LE(address+1)] << 8) |
			(OP_ROM[BYTE4_XOR_LE(address+2)] << 16) | (OP_ROM[BYTE4_XOR_LE(address+3)] << 24);
}

static void ChangePC_32(offs_t pc)
{
	change_pc32ledw(pc);
}



/************************************************/
/* Structures pointing to various I/O functions */
/************************************************/

static struct cpu_info v60_i =
{
	MemRead8_16,  MemWrite8_16,  MemRead16_16,  MemWrite16_16,  MemRead32_16,  MemWrite32_16,
	PortRead8_16, PortWrite8_16, PortRead16_16, PortWrite16_16, PortRead32_16, PortWrite32_16,
	OpRead8_16,                  OpRead16_16,                   OpRead32_16,
	ChangePC_16,
	0xfffff0
};

static struct cpu_info v70_i =
{
	MemRead8_32,  MemWrite8_32,  MemRead16_32,  MemWrite16_32,  MemRead32_32,  MemWrite32_32,
	PortRead8_32, PortWrite8_32, PortRead16_32, PortWrite16_32, PortRead32_32, PortWrite32_32,
	OpRead8_32,                  OpRead16_32,                   OpRead32_32,
	ChangePC_32,
	0xfffffff0
};



/**************************************/
/* Macro shorthands for I/O functions */
/**************************************/

#define MemRead8    v60.info.mr8
#define MemWrite8   v60.info.mw8
#define MemRead16   v60.info.mr16
#define MemWrite16  v60.info.mw16
#define MemRead32   v60.info.mr32
#define MemWrite32  v60.info.mw32

#define PortRead8   v60.info.pr8
#define PortWrite8  v60.info.pw8
#define PortRead16  v60.info.pr16
#define PortWrite16 v60.info.pw16
#define PortRead32  v60.info.pr32
#define PortWrite32 v60.info.pw32

#if !defined(MSB_FIRST) && !defined(ALIGN_INTS)
#define OpRead8(a)	(OP_ROM[a])
#define OpRead16(a)	(*(UINT16 *)&OP_ROM[a])
#define OpRead32(a)	(*(UINT32 *)&OP_ROM[a])
#else
#define OpRead8     v60.info.mr8
#define OpRead16    v60.info.mr16
#define OpRead32    v60.info.mr32
#endif

#define ChangePC	v60.info.chpc
