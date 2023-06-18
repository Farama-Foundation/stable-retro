/*###################################################################################################
**
**	TMS34010: Portable Texas Instruments TMS34010 emulator
**
**	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998
**	 Parts based on code by Aaron Giles
**
**#################################################################################################*/

#include <stdio.h>
#include "driver.h"
#include "osd_cpu.h"
#include "mamedbg.h"
#include "tms34010.h"
#include "34010ops.h"


/*###################################################################################################
**	FIELD WRITE FUNCTIONS
**#################################################################################################*/

void wfield_01(offs_t offset,data32_t data)
{
	WFIELDMAC(0x01,16);
}

void wfield_02(offs_t offset,data32_t data)
{
	WFIELDMAC(0x03,15);
}

void wfield_03(offs_t offset,data32_t data)
{
	WFIELDMAC(0x07,14);
}

void wfield_04(offs_t offset,data32_t data)
{
	WFIELDMAC(0x0f,13);
}

void wfield_05(offs_t offset,data32_t data)
{
	WFIELDMAC(0x1f,12);
}

void wfield_06(offs_t offset,data32_t data)
{
	WFIELDMAC(0x3f,11);
}

void wfield_07(offs_t offset,data32_t data)
{
	WFIELDMAC(0x7f,10);
}

void wfield_08(offs_t offset,data32_t data)
{
	WFIELDMAC_8;
}

void wfield_09(offs_t offset,data32_t data)
{
	WFIELDMAC(0x1ff,8);
}

void wfield_10(offs_t offset,data32_t data)
{
	WFIELDMAC(0x3ff,7);
}

void wfield_11(offs_t offset,data32_t data)
{
	WFIELDMAC(0x7ff,6);
}

void wfield_12(offs_t offset,data32_t data)
{
	WFIELDMAC(0xfff,5);
}

void wfield_13(offs_t offset,data32_t data)
{
	WFIELDMAC(0x1fff,4);
}

void wfield_14(offs_t offset,data32_t data)
{
	WFIELDMAC(0x3fff,3);
}

void wfield_15(offs_t offset,data32_t data)
{
	WFIELDMAC(0x7fff,2);
}

void wfield_16(offs_t offset,data32_t data)
{
	if (offset & 0x0f)
	{
		WFIELDMAC(0xffff,1);
	}
	else
	{
		TMS34010_WRMEM_WORD(TOBYTE(offset),data);
	}
}

void wfield_17(offs_t offset,data32_t data)
{
	WFIELDMAC(0x1ffff,0);
}

void wfield_18(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0x3ffff,15);
}

void wfield_19(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0x7ffff,14);
}

void wfield_20(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0xfffff,13);
}

void wfield_21(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0x1fffff,12);
}

void wfield_22(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0x3fffff,11);
}

void wfield_23(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0x7fffff,10);
}

void wfield_24(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0xffffff,9);
}

void wfield_25(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0x1ffffff,8);
}

void wfield_26(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0x3ffffff,7);
}

void wfield_27(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0x7ffffff,6);
}

void wfield_28(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0xfffffff,5);
}

void wfield_29(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0x1fffffff,4);
}

void wfield_30(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0x3fffffff,3);
}

void wfield_31(offs_t offset,data32_t data)
{
	WFIELDMAC_BIG(0x7fffffff,2);
}

void wfield_32(offs_t offset,data32_t data)
{
	WFIELDMAC_32;
}



/*###################################################################################################
**	FIELD READ FUNCTIONS (ZERO-EXTEND)
**#################################################################################################*/

data32_t rfield_z_01(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x01,16);
	return ret;
}

data32_t rfield_z_02(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x03,15);
	return ret;
}

data32_t rfield_z_03(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x07,14);
	return ret;
}

data32_t rfield_z_04(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x0f,13);
	return ret;
}

data32_t rfield_z_05(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x1f,12);
	return ret;
}

data32_t rfield_z_06(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x3f,11);
	return ret;
}

data32_t rfield_z_07(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x7f,10);
	return ret;
}

data32_t rfield_z_08(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_8;
	return ret;
}

data32_t rfield_z_09(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x1ff,8);
	return ret;
}

data32_t rfield_z_10(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x3ff,7);
	return ret;
}

data32_t rfield_z_11(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x7ff,6);
	return ret;
}

data32_t rfield_z_12(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0xfff,5);
	return ret;
}

data32_t rfield_z_13(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x1fff,4);
	return ret;
}

data32_t rfield_z_14(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x3fff,3);
	return ret;
}

data32_t rfield_z_15(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x7fff,2);
	return ret;
}

data32_t rfield_z_16(offs_t offset)
{
	UINT32 ret;
	if (offset & 0x0f)
	{
		RFIELDMAC(0xffff,1);
	}

	else
		ret = TMS34010_RDMEM_WORD(TOBYTE(offset));
	return ret;
}

data32_t rfield_z_17(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x1ffff,0);
	return ret;
}

data32_t rfield_z_18(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x3ffff,15);
	return ret;
}

data32_t rfield_z_19(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x7ffff,14);
	return ret;
}

data32_t rfield_z_20(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0xfffff,13);
	return ret;
}

data32_t rfield_z_21(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x1fffff,12);
	return ret;
}

data32_t rfield_z_22(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x3fffff,11);
	return ret;
}

data32_t rfield_z_23(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x7fffff,10);
	return ret;
}

data32_t rfield_z_24(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0xffffff,9);
	return ret;
}

data32_t rfield_z_25(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x1ffffff,8);
	return ret;
}

data32_t rfield_z_26(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x3ffffff,7);
	return ret;
}

data32_t rfield_z_27(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x7ffffff,6);
	return ret;
}

data32_t rfield_z_28(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0xfffffff,5);
	return ret;
}

data32_t rfield_z_29(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x1fffffff,4);
	return ret;
}

data32_t rfield_z_30(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x3fffffff,3);
	return ret;
}

data32_t rfield_z_31(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x7fffffff,2);
	return ret;
}

data32_t rfield_32(offs_t offset)
{
	RFIELDMAC_32;
}



/*###################################################################################################
**	FIELD READ FUNCTIONS (SIGN-EXTEND)
**#################################################################################################*/

data32_t rfield_s_01(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x01,16);
	return ((INT32)(ret << 31)) >> 31;
}

data32_t rfield_s_02(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x03,15);
	return ((INT32)(ret << 30)) >> 30;
}

data32_t rfield_s_03(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x07,14);
	return ((INT32)(ret << 29)) >> 29;
}

data32_t rfield_s_04(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x0f,13);
	return ((INT32)(ret << 28)) >> 28;
}

data32_t rfield_s_05(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x1f,12);
	return ((INT32)(ret << 27)) >> 27;
}

data32_t rfield_s_06(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x3f,11);
	return ((INT32)(ret << 26)) >> 26;
}

data32_t rfield_s_07(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x7f,10);
	return ((INT32)(ret << 25)) >> 25;
}

data32_t rfield_s_08(offs_t offset)
{
	UINT32 ret;
	if (offset & 0x07)
	{
		RFIELDMAC(0xff,9);
	}

	else
		ret = TMS34010_RDMEM(TOBYTE(offset));
	return (INT32)(INT8)ret;
}

data32_t rfield_s_09(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x1ff,8);
	return ((INT32)(ret << 23)) >> 23;
}

data32_t rfield_s_10(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x3ff,7);
	return ((INT32)(ret << 22)) >> 22;
}

data32_t rfield_s_11(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x7ff,6);
	return ((INT32)(ret << 21)) >> 21;
}

data32_t rfield_s_12(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0xfff,5);
	return ((INT32)(ret << 20)) >> 20;
}

data32_t rfield_s_13(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x1fff,4);
	return ((INT32)(ret << 19)) >> 19;
}

data32_t rfield_s_14(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x3fff,3);
	return ((INT32)(ret << 18)) >> 18;
}

data32_t rfield_s_15(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x7fff,2);
	return ((INT32)(ret << 17)) >> 17;
}

data32_t rfield_s_16(offs_t offset)
{
	UINT32 ret;
	if (offset & 0x0f)
	{
		RFIELDMAC(0xffff,1);
	}

	else
	{
		ret = TMS34010_RDMEM_WORD(TOBYTE(offset));
	}

	return (INT32)(INT16)ret;
}

data32_t rfield_s_17(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC(0x1ffff,0);
	return ((INT32)(ret << 15)) >> 15;
}

data32_t rfield_s_18(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x3ffff,15);
	return ((INT32)(ret << 14)) >> 14;
}

data32_t rfield_s_19(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x7ffff,14);
	return ((INT32)(ret << 13)) >> 13;
}

data32_t rfield_s_20(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0xfffff,13);
	return ((INT32)(ret << 12)) >> 12;
}

data32_t rfield_s_21(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x1fffff,12);
	return ((INT32)(ret << 11)) >> 11;
}

data32_t rfield_s_22(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x3fffff,11);
	return ((INT32)(ret << 10)) >> 10;
}

data32_t rfield_s_23(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x7fffff,10);
	return ((INT32)(ret << 9)) >> 9;
}

data32_t rfield_s_24(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0xffffff,9);
	return ((INT32)(ret << 8)) >> 8;
}

data32_t rfield_s_25(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x1ffffff,8);
	return ((INT32)(ret << 7)) >> 7;
}

data32_t rfield_s_26(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x3ffffff,7);
	return ((INT32)(ret << 6)) >> 6;
}

data32_t rfield_s_27(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x7ffffff,6);
	return ((INT32)(ret << 5)) >> 5;
}

data32_t rfield_s_28(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0xfffffff,5);
	return ((INT32)(ret << 4)) >> 4;
}

data32_t rfield_s_29(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x1fffffff,4);
	return ((INT32)(ret << 3)) >> 3;
}

data32_t rfield_s_30(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x3fffffff,3);
	return ((INT32)(ret << 2)) >> 2;
}

data32_t rfield_s_31(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_BIG(0x7fffffff,2);
	return ((INT32)(ret << 1)) >> 1;
}
