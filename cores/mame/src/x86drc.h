/*###################################################################################################
**
**
**		drccore.h
**		x86 Dynamic recompiler support routines.
**		Written by Aaron Giles
**
**
**#################################################################################################*/

#ifndef __DRCCORE_H__
#define __DRCCORE_H__


/*###################################################################################################
**	TYPE DEFINITIONS
**#################################################################################################*/

/* PC and pointer pair */
struct pc_ptr_pair
{
	UINT32		pc;
	UINT8 *		target;
};

/* core interface structure for the drc common code */
struct drccore
{
	UINT8 *		cache_base;				/* base pointer to the compiler cache */
	UINT8 *		cache_top;				/* current top of cache */
	UINT8 *		cache_danger;			/* high water mark for the end */
	UINT8 *		cache_end;				/* end of cache memory */

	void ***	lookup_l1;				/* level 1 lookup */
	void **		lookup_l2_recompile;	/* level 2 lookup populated with recompile pointers */
	UINT8		l1bits;					/* number of bits in level 1 lookup */
	UINT8		l2bits;					/* number of bits in level 2 lookup */
	UINT8		l1shift;				/* shift to go from PC to level 1 lookup */
	UINT32		l2mask;					/* mask to go from PC to level 2 lookup */
	UINT8		l2scale;				/* scale to get from masked PC value to final level 2 lookup */

	void 		(*entry_point)(void);	/* pointer to asm entry point */
	void *		out_of_cycles;			/* pointer to out of cycles jump point */
	void *		recompile;				/* pointer to recompile jump point */
	void *		dispatch;				/* pointer to dispatch jump point */

	UINT32 *	pcptr;					/* pointer to where the PC is stored */
	UINT32 *	icountptr;				/* pointer to where the icount is stored */
	UINT32 *	esiptr;					/* pointer to where the volatile data in ESI is stored */

	UINT8		uses_fp;				/* true if we need the FP unit */
	UINT8		uses_sse;				/* true if we need the SSE unit */
	UINT16		fpcw_curr;				/* current FPU control word */
	UINT32		mcrxr_curr;				/* current SSE control word */
	UINT16		fpcw_save;				/* saved FPU control word */
	UINT32		mcrxr_save;				/* saved SSE control word */
	
	struct pc_ptr_pair *sequence_list;	/* PC/pointer sets for the current instruction sequence */
	UINT32		sequence_count;			/* number of instructions in the current sequence */
	UINT32		sequence_count_max;		/* max number of instructions in the current sequence */
	struct pc_ptr_pair *tentative_list;	/* PC/pointer sets for tentative branches */
	UINT32		tentative_count;		/* number of tentative branches */
	UINT32		tentative_count_max;	/* max number of tentative branches */

	void 		(*cb_reset)(struct drccore *drc);		/* callback when the cache is reset */
	void 		(*cb_recompile)(struct drccore *drc);	/* callback when code needs to be recompiled */
	void 		(*cb_entrygen)(struct drccore *drc);	/* callback before generating the dispatcher on entry */
};

/* configuration structure for the drc common code */
struct drcconfig
{
	UINT32		cache_size;				/* size of cache to allocate */
	UINT32		max_instructions;		/* maximum instructions per sequence */
	UINT8		address_bits;			/* number of live address bits in the PC */
	UINT8		lsbs_to_ignore;			/* number of LSBs to ignore on the PC */
	UINT8		uses_fp;				/* true if we need the FP unit */
	UINT8		uses_sse;				/* true if we need the SSE unit */

	UINT32 *	pcptr;					/* pointer to where the PC is stored */
	UINT32 *	icountptr;				/* pointer to where the icount is stored */
	UINT32 *	esiptr;					/* pointer to where the volatile data in ESI is stored */

	void 		(*cb_reset)(struct drccore *drc);		/* callback when the cache is reset */
	void 		(*cb_recompile)(struct drccore *drc);	/* callback when code needs to be recompiled */
	void 		(*cb_entrygen)(struct drccore *drc);	/* callback before generating the dispatcher on entry */
};

/* structure to hold link data to be filled in later */
struct linkdata
{
	UINT8 		size;
	UINT8 *		target;
};



/*###################################################################################################
**	HELPER MACROS
**#################################################################################################*/

/* useful macros for accessing hi/lo portions of 64-bit values */
#define LO(x)		(&(((UINT32 *)(x))[0]))
#define HI(x)		(&(((UINT32 *)(x))[1]))

extern const UINT8 scale_lookup[];


/*###################################################################################################
**	CONSTANTS
**#################################################################################################*/

/* architectural defines */
#define REG_EAX		0
#define REG_ECX		1
#define REG_EDX		2
#define REG_EBX		3
#define REG_ESP		4
#define REG_EBP		5
#define REG_ESI		6
#define REG_EDI		7

#define REG_AX		0
#define REG_CX		1
#define REG_DX		2
#define REG_BX		3
#define REG_SP		4
#define REG_BP		5
#define REG_SI		6
#define REG_DI		7

#define REG_AL		0
#define REG_CL		1
#define REG_DL		2
#define REG_BL		3
#define REG_AH		4
#define REG_CH		5
#define REG_DH		6
#define REG_BH		7

#define REG_XMM0	0
#define REG_XMM1	1
#define REG_XMM2	2
#define REG_XMM3	3
#define REG_XMM4	4
#define REG_XMM5	5
#define REG_XMM6	6
#define REG_XMM7	7

#define NO_BASE		5

#define COND_A		7
#define COND_AE		3
#define COND_B		2
#define COND_BE		6
#define COND_C		2
#define COND_E		4
#define COND_Z		4
#define COND_G		15
#define COND_GE		13
#define COND_L		12
#define COND_LE		14
#define COND_NA		6
#define COND_NAE	2
#define COND_NB		3
#define COND_NBE	7
#define COND_NC		3
#define COND_NE		5
#define COND_NG		14
#define COND_NGE	12
#define COND_NL		13
#define COND_NLE	15
#define COND_NO		1
#define COND_NP		11
#define COND_NS		9
#define COND_NZ		5
#define COND_O		0
#define COND_P		10
#define COND_PE		10
#define COND_PO		11
#define COND_S		8
#define COND_Z		4

/* rounding modes */
#define FPRND_NEAR	0
#define FPRND_DOWN	1
#define FPRND_UP	2
#define FPRND_CHOP	3



/*###################################################################################################
**	LOW-LEVEL OPCODE EMITTERS
**#################################################################################################*/

/* lowest-level opcode emitters */
#define OP1(x)		do { *drc->cache_top++ = (UINT8)(x); } while (0)
#define OP2(x)		do { *(UINT16 *)drc->cache_top = (UINT16)(x); drc->cache_top += 2; } while (0)
#define OP4(x)		do { *(UINT32 *)drc->cache_top = (UINT32)(x); drc->cache_top += 4; } while (0)



/*###################################################################################################
**	MODRM EMITTERS
**#################################################################################################*/

/* op  reg,reg*/
#define MODRM_REG(reg, rm) 		\
do { OP1(0xc0 | (((reg) & 7) << 3) | ((rm) & 7)); } while (0)

/* op  reg,[addr]*/
#define MODRM_MABS(reg, addr)	\
do { OP1(0x05 | (((reg) & 7) << 3)); OP4(addr); } while (0)

/* op  reg,[base+disp]*/
#define MODRM_MBD(reg, base, disp) \
do {														\
	if ((UINT32)(disp) == 0 && (base) != REG_ESP && (base) != REG_EBP) \
	{														\
		OP1(0x00 | (((reg) & 7) << 3) | ((base) & 7));		\
	}														\
	else if ((INT8)(INT32)(disp) == (INT32)(disp))			\
	{														\
		if ((base) == REG_ESP)								\
		{													\
			OP1(0x44 | (((reg) & 7) << 3));					\
			OP1(0x24);										\
			OP1((INT32)disp);								\
		}													\
		else												\
		{													\
			OP1(0x40 | (((reg) & 7) << 3) | ((base) & 7));	\
			OP1((INT32)disp);								\
		}													\
	}														\
	else													\
	{														\
		if ((base) == REG_ESP)								\
		{													\
			OP1(0x84 | (((reg) & 7) << 3));					\
			OP1(0x24);										\
			OP4(disp);										\
		}													\
		else												\
		{													\
			OP1(0x80 | (((reg) & 7) << 3) | ((base) & 7));	\
			OP4(disp);										\
		}													\
	}														\
} while (0)

/* op  reg,[base+indx*scale+disp]*/
#define MODRM_MBISD(reg, base, indx, scale, disp)			\
do {														\
	if ((scale) == 1 && (base) == NO_BASE)					\
		MODRM_MBD(reg,indx,disp);							\
	else if ((UINT32)(disp) == 0 || (base) == NO_BASE)		\
	{														\
		OP1(0x04 | (((reg) & 7) << 3));						\
		OP1((scale_lookup[scale] << 6) | (((indx) & 7) << 3) | ((base) & 7));\
		if ((UINT32)(disp) != 0) OP4(disp);					\
	}														\
	else if ((INT8)(INT32)(disp) == (INT32)(disp))			\
	{														\
		OP1(0x44 | (((reg) & 7) << 3));						\
		OP1((scale_lookup[scale] << 6) | (((indx) & 7) << 3) | ((base) & 7));\
		OP1((INT32)disp);									\
	}														\
	else													\
	{														\
		OP1(0x84 | (((reg) & 7) << 3));						\
		OP1((scale_lookup[scale] << 6) | (((indx) & 7) << 3) | ((base) & 7));\
		OP4(disp);											\
	}														\
} while (0)



/*###################################################################################################
**	SIMPLE OPCODE EMITTERS
**#################################################################################################*/

#define _pushad() \
do { OP1(0x60); } while (0)

#define _push_r32(reg) \
do { OP1(0x50+(reg)); } while (0)

#define _push_imm(imm) \
do { OP1(0x68); OP4(imm); } while (0)

#define _push_m32abs(addr) \
do { OP1(0xff); MODRM_MABS(6, addr); } while (0)

#define _popad() \
do { OP1(0x61); } while (0)

#define _pop_r32(reg) \
do { OP1(0x58+(reg)); } while (0)

#define _pop_m32abs(addr) \
do { OP1(0x8f); MODRM_MABS(0, addr); } while (0)

#define _ret() \
do { OP1(0xc3); } while (0)

#define _cdq() \
do { OP1(0x99); } while (0)

#define _lahf() \
do { OP1(0x9F); } while(0);

#define _sahf() \
do { OP1(0x9E); } while(0);



/*###################################################################################################
**	MOVE EMITTERS
**#################################################################################################*/

#define _mov_r8_imm(dreg, imm) \
do { OP1(0xb0 + (dreg)); OP1(imm); } while (0)

#define _mov_r8_r8(dreg, sreg) \
do { OP1(0x8a); MODRM_REG(dreg, sreg); } while (0)

#define _mov_r8_m8abs(dreg, addr) \
do { OP1(0x8a); MODRM_MABS(dreg, addr); } while (0)

#define _mov_r8_m8bd(dreg, base, disp) \
do { OP1(0x8a); MODRM_MBD(dreg, base, disp); } while (0)

#define _mov_r8_m8isd(dreg, indx, scale, disp) \
do { OP1(0x8a); MODRM_MBISD(dreg, NO_BASE, indx, scale, disp); } while (0)

#define _mov_r8_m8bisd(dreg, base, indx, scale, disp) \
do { OP1(0x8a); MODRM_MBISD(dreg, base, indx, scale, disp); } while (0)



#define _mov_r16_imm(dreg, imm) \
do { OP1(0x66); OP1(0xb8 + (dreg)); OP2(imm); } while (0)

#define _mov_r16_r16(dreg, sreg) \
do { OP1(0x66); OP1(0x8b); MODRM_REG(dreg, sreg); } while (0)

#define _mov_r16_m16abs(dreg, addr) \
do { OP1(0x66); OP1(0x8b); MODRM_MABS(dreg, addr); } while (0)

#define _mov_r16_m16bd(dreg, base, disp) \
do { OP1(0x66); OP1(0x8b); MODRM_MBD(dreg, base, disp); } while (0)

#define _mov_r16_m16isd(dreg, indx, scale, disp) \
do { OP1(0x66); OP1(0x8b); MODRM_MBISD(dreg, NO_BASE, indx, scale, disp); } while (0)

#define _mov_r16_m16bisd(dreg, base, indx, scale, disp) \
do { OP1(0x66); OP1(0x8b); MODRM_MBISD(dreg, base, indx, scale, disp); } while (0)



#define _mov_r32_imm(dreg, imm) \
do { OP1(0xb8 + (dreg)); OP4(imm); } while (0)

#define _mov_r32_r32(dreg, sreg) \
do { OP1(0x8b); MODRM_REG(dreg, sreg); } while (0)

#define _mov_r32_m32abs(dreg, addr) \
do { OP1(0x8b); MODRM_MABS(dreg, addr); } while (0)

#define _mov_r32_m32bd(dreg, base, disp) \
do { OP1(0x8b); MODRM_MBD(dreg, base, disp); } while (0)

#define _mov_r32_m32isd(dreg, indx, scale, disp) \
do { OP1(0x8b); MODRM_MBISD(dreg, NO_BASE, indx, scale, disp); } while (0)

#define _mov_r32_m32bisd(dreg, base, indx, scale, disp) \
do { OP1(0x8b); MODRM_MBISD(dreg, base, indx, scale, disp); } while (0)



#define _mov_m8abs_imm(addr, imm) \
do { OP1(0xc6); MODRM_MABS(0, addr); OP1(imm); } while (0)

#define _mov_m8abs_r8(addr, sreg) \
do { OP1(0x88); MODRM_MABS(sreg, addr); } while (0)

#define _mov_m8bd_r8(base, disp, sreg) \
do { OP1(0x88); MODRM_MBD(sreg, base, disp); } while (0)

#define _mov_m8isd_r8(indx, scale, disp, sreg) \
do { OP1(0x88); MODRM_MBISD(sreg, NO_BASE, indx, scale, disp); } while (0)

#define _mov_m8bisd_r8(base, indx, scale, disp, sreg) \
do { OP1(0x88); MODRM_MBISD(sreg, base, indx, scale, disp); } while (0)



#define _mov_m16abs_imm(addr, imm) \
do { OP1(0x66); OP1(0xc7); MODRM_MABS(0, addr); OP2(imm); } while (0)

#define _mov_m16abs_r16(addr, sreg) \
do { OP1(0x66); OP1(0x89); MODRM_MABS(sreg, addr); } while (0)

#define _mov_m16bd_r16(base, disp, sreg) \
do { OP1(0x66); OP1(0x89); MODRM_MBD(sreg, base, disp); } while (0)




#define _mov_m32abs_imm(addr, imm) \
do { OP1(0xc7); MODRM_MABS(0, addr); OP4(imm); } while (0)

#define _mov_m32bisd_imm(base, indx, scale, addr, imm) \
do { OP1(0xc7); MODRM_MBISD(0, base, indx, scale, addr); OP4(imm); } while (0)

#define _mov_m32bd_r32(base, disp, sreg) \
do { OP1(0x89); MODRM_MBD(sreg, base, disp); } while (0)



#define _mov_m32abs_r32(addr, sreg) \
do { OP1(0x89); MODRM_MABS(sreg, addr); } while (0)

#define _mov_m32isd_r32(indx, scale, addr, dreg) \
do { OP1(0x89); MODRM_MBISD(dreg, NO_BASE, indx, scale, addr); } while (0)

#define _mov_m32bisd_r32(base, indx, scale, addr, dreg) \
do { OP1(0x89); MODRM_MBISD(dreg, base, indx, scale, addr); } while (0)



#define _mov_r64_m64abs(reghi, reglo, addr) \
do { _mov_r32_m32abs(reglo, LO(addr)); _mov_r32_m32abs(reghi, HI(addr)); } while (0)

#define _mov_m64abs_r64(addr, reghi, reglo) \
do { _mov_m32abs_r32(LO(addr), reglo); _mov_m32abs_r32(HI(addr), reghi); } while (0)

#define _mov_m64abs_imm32(addr, imm) \
do { _mov_m32abs_imm(LO(addr), imm); _mov_m32abs_imm(HI(addr), ((INT32)(imm) >> 31)); } while (0)



#define _movsx_r32_r8(dreg, sreg) \
do { OP1(0x0f); OP1(0xbe); MODRM_REG(dreg, sreg); } while (0)

#define _movsx_r32_r16(dreg, sreg) \
do { OP1(0x0f); OP1(0xbf); MODRM_REG(dreg, sreg); } while (0)

#define _movsx_r32_m8abs(dreg, addr) \
do { OP1(0x0f); OP1(0xbe); MODRM_MABS(dreg, addr); } while (0)

#define _movsx_r32_m16abs(dreg, addr) \
do { OP1(0x0f); OP1(0xbf); MODRM_MABS(dreg, addr); } while (0)

#define _movzx_r32_r8(dreg, sreg) \
do { OP1(0x0f); OP1(0xb6); MODRM_REG(dreg, sreg); } while (0)

#define _movzx_r32_r16(dreg, sreg) \
do { OP1(0x0f); OP1(0xb7); MODRM_REG(dreg, sreg); } while (0)

#define _movzx_r32_m8abs(dreg, addr) \
do { OP1(0x0f); OP1(0xb6); MODRM_MABS(dreg, addr); } while (0)

#define _movzx_r32_m16abs(dreg, addr) \
do { OP1(0x0f); OP1(0xb7); MODRM_MABS(dreg, addr); } while (0)



#define _lea_r32_m32bd(dest, base, disp) \
do { OP1(0x8d); MODRM_MBD(dest, base, disp); } while (0)

#define _lea_r32_m32isd(dest, indx, scale, disp) \
do { OP1(0x8d); MODRM_MBISD(dest, NO_BASE, indx, scale, disp); } while (0)



/*###################################################################################################
**	SHIFT EMITTERS
**#################################################################################################*/

#define _sar_r32_cl(dreg) \
do { OP1(0xd3); MODRM_REG(7, dreg); } while (0)

#define _shl_r32_cl(dreg) \
do { OP1(0xd3); MODRM_REG(4, dreg); } while (0)

#define _shr_r32_cl(dreg) \
do { OP1(0xd3); MODRM_REG(5, dreg); } while (0)

#define _sar_r32_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0xd1); MODRM_REG(7, dreg); } \
	else { OP1(0xc1); MODRM_REG(7, dreg); OP1(imm); } \
} while (0)

#define _shl_r32_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0xd1); MODRM_REG(4, dreg); } \
	else { OP1(0xc1); MODRM_REG(4, dreg); OP1(imm); } \
} while (0)

#define _shr_r32_imm(dreg, imm) \
do { \
	if ((imm) == 1) { OP1(0xd1); MODRM_REG(5, dreg); } \
	else { OP1(0xc1); MODRM_REG(5, dreg); OP1(imm); } \
} while (0)



#define _shld_r32_r32_cl(dreg, sreg) \
do { OP1(0x0f); OP1(0xa5); MODRM_REG(sreg, dreg); } while (0)

#define _shld_r32_r32_imm(dreg, sreg, imm) \
do { OP1(0x0f); OP1(0xa4); MODRM_REG(sreg, dreg); OP1(imm); } while (0)

#define _shrd_r32_r32_cl(dreg, sreg) \
do { OP1(0x0f); OP1(0xad); MODRM_REG(sreg, dreg); } while (0)

#define _shrd_r32_r32_imm(dreg, sreg, imm) \
do { OP1(0x0f); OP1(0xac); MODRM_REG(sreg, dreg); OP1(imm); } while (0)



/*###################################################################################################
**	UNARY ARITHMETIC EMITTERS
**#################################################################################################*/

#define _neg_r32(reg) \
do { OP1(0xf7); MODRM_REG(3, reg); } while (0)

#define _not_r32(reg) \
do { OP1(0xf7); MODRM_REG(2, reg); } while (0)



/*###################################################################################################
**	32-BIT ARITHMETIC EMITTERS
**#################################################################################################*/

#define _add_r32_r32(r1, r2) \
do { OP1(0x01); MODRM_REG(r2, r1); } while (0)

#define _adc_r32_r32(r1, r2) \
do { OP1(0x11); MODRM_REG(r2, r1); } while (0)

#define _or_r32_r32(r1, r2) \
do { OP1(0x09); MODRM_REG(r2, r1); } while (0)

#define _sub_r32_r32(r1, r2) \
do { OP1(0x29); MODRM_REG(r2, r1); } while (0)

#define _sbb_r32_r32(r1, r2) \
do { OP1(0x19); MODRM_REG(r2, r1); } while (0)

#define _xor_r32_r32(r1, r2) \
do { OP1(0x31); MODRM_REG(r2, r1); } while (0)

#define _or_r8_r8(r1, r2) \
	do { OP1(0x0A); MODRM_REG(r2, r1); } while (0)



#define _add_r32_m32abs(dreg, addr) \
do { OP1(0x03); MODRM_MABS(dreg, addr); } while (0)

#define _adc_r32_m32abs(dreg, addr) \
do { OP1(0x13); MODRM_MABS(dreg, addr); } while (0)

#define _and_r32_m32abs(dreg, addr) \
do { OP1(0x23); MODRM_MABS(dreg, addr); } while (0)

#define _cmp_r32_m32abs(dreg, addr) \
do { OP1(0x3b); MODRM_MABS(dreg, addr); } while (0)

#define _or_r32_m32abs(dreg, addr) \
do { OP1(0x0b); MODRM_MABS(dreg, addr); } while (0)

#define _sub_r32_m32abs(dreg, addr) \
do { OP1(0x2b); MODRM_MABS(dreg, addr); } while (0)

#define _sbb_r32_m32abs(dreg, addr) \
do { OP1(0x1b); MODRM_MABS(dreg, addr); } while (0)

#define _xor_r32_m32abs(dreg, addr) \
do { OP1(0x33); MODRM_MABS(dreg, addr); } while (0)



#define _arith_r32_imm_common(reg, dreg, imm)		\
do {												\
	if ((INT8)(imm) == (INT32)(imm))				\
	{												\
		OP1(0x83); MODRM_REG(reg, dreg); OP1(imm);	\
	}												\
	else											\
	{												\
		OP1(0x81); MODRM_REG(reg, dreg); OP4(imm);	\
	}												\
} while (0)

#define _add_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(0, dreg, imm); } while (0)

#define _adc_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(2, dreg, imm); } while (0)

#define _or_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(1, dreg, imm); } while (0)

#define _sbb_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(3, dreg, imm); } while (0)

#define _and_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(4, dreg, imm); } while (0)

#define _sub_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(5, dreg, imm); } while (0)

#define _xor_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(6, dreg, imm); } while (0)

#define _cmp_r32_imm(dreg, imm) \
do { _arith_r32_imm_common(7, dreg, imm); } while (0)

#define _test_r32_imm(dreg, imm) \
do { OP1(0xf7); MODRM_REG(0, dreg); OP4(imm); } while (0)

#define _and_r32_r32(dreg, sreg) \
do { OP1(0x23); MODRM_REG(dreg, sreg); } while (0)




#define _arith_m32abs_imm_common(reg, addr, imm)	\
do {												\
	if ((INT8)(imm) == (INT32)(imm))				\
	{												\
		OP1(0x83); MODRM_MABS(reg, addr); OP1(imm);	\
	}												\
	else											\
	{												\
		OP1(0x81); MODRM_MABS(reg, addr); OP4(imm);	\
	}												\
} while (0)

#define _add_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(0, addr, imm); } while (0)

#define _adc_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(2, addr, imm); } while (0)

#define _or_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(1, addr, imm); } while (0)

#define _sbb_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(3, addr, imm); } while (0)

#define _and_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(4, addr, imm); } while (0)

#define _sub_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(5, addr, imm); } while (0)

#define _xor_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(6, addr, imm); } while (0)

#define _cmp_m32abs_imm(addr, imm) \
do { _arith_m32abs_imm_common(7, addr, imm); } while (0)

#define _test_m32abs_imm(addr, imm) \
do { OP1(0xf7); MODRM_MABS(0, addr); OP4(imm); } while (0)



#define _arith_m32bd_imm_common(reg, base, disp, imm)	\
do {												\
	if ((INT8)(imm) == (INT32)(imm))				\
	{												\
		OP1(0x83); MODRM_MBD(reg, base, disp); OP1(imm);\
	}												\
	else											\
	{												\
		OP1(0x81); MODRM_MBD(reg, base, disp); OP4(imm);\
	}												\
} while (0)

#define _add_m32bd_imm(base, disp, imm) \
do { _arith_m32bd_imm_common(0, base, disp, imm); } while (0)



#define _and_r32_m32bd(dreg, base, disp) \
do { OP1(0x23); MODRM_MBD(dreg, base, disp); } while (0)



#define _imul_r32(reg) \
do { OP1(0xf7); MODRM_REG(5, reg); } while (0)

#define _mul_r32(reg) \
do { OP1(0xf7); MODRM_REG(4, reg); } while (0)

#define _mul_m32abs(addr) \
do { OP1(0xf7); MODRM_MABS(4, addr); } while (0)

#define _idiv_r32(reg) \
do { OP1(0xf7); MODRM_REG(7, reg); } while (0)

#define _div_r32(reg) \
do { OP1(0xf7); MODRM_REG(6, reg); } while (0)

#define _and_m32bd_r32(base, disp, sreg) \
do { OP1(0x21); MODRM_MBD(sreg, base, disp); } while (0)

#define _add_m32bd_r32(base, disp, sreg) \
do { OP1(0x89); MODRM_MBD(sreg, base, disp); } while (0)

#define _sub_m32bd_r32(base, disp, sreg) \
do { OP1(0x29); MODRM_MBD(sreg, base, disp); } while (0)

#define _rol_r32_cl(reg) \
do { OP1(0xd3);	OP1(0xc0 | ((reg) & 7)); } while(0)

#define _ror_r32_cl(reg) \
do { OP1(0xd3);	OP1(0xc8 | ((reg) & 7)); } while(0)

#define _rcl_r32_cl(reg) \
do { OP1(0xd3);	OP1(0xd0 | ((reg) & 7)); } while(0)

#define _rcr_r32_cl(reg) \
do { OP1(0xd3);	OP1(0xd8 | ((reg) & 7)); } while(0)




/*###################################################################################################
**	16-BIT AND 8-BIT ARITHMETIC EMITTERS
**#################################################################################################*/

#define _arith_m16abs_imm_common(reg, addr, imm)	\
do {												\
	OP1(0x66);										\
	if ((INT8)(imm) == (INT16)(imm))				\
	{												\
		OP1(0x83); MODRM_MABS(reg, addr); OP1(imm);	\
	}												\
	else											\
	{												\
		OP1(0x81); MODRM_MABS(reg, addr); OP2(imm);	\
	}												\
} while (0)

#define _add_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(0, addr, imm); } while (0)

#define _or_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(1, addr, imm); } while (0)

#define _sbb_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(3, addr, imm); } while (0)

#define _and_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(4, addr, imm); } while (0)

#define _sub_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(5, addr, imm); } while (0)

#define _xor_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(6, addr, imm); } while (0)

#define _cmp_m16abs_imm(addr, imm) \
do { _arith_m16abs_imm_common(7, addr, imm); } while (0)

#define _test_m16abs_imm(addr, imm) \
do { OP1(0xf7); MODRM_MABS(0, addr); OP2(imm); } while (0)



#define _arith_m8abs_imm_common(reg, addr, imm)		\
do { OP1(0x80); MODRM_MABS(reg, addr); OP1(imm); } while (0)

#define _add_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(0, addr, imm); } while (0)

#define _or_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(1, addr, imm); } while (0)

#define _sbb_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(3, addr, imm); } while (0)

#define _and_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(4, addr, imm); } while (0)

#define _sub_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(5, addr, imm); } while (0)

#define _xor_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(6, addr, imm); } while (0)

#define _cmp_m8abs_imm(addr, imm) \
do { _arith_m8abs_imm_common(7, addr, imm); } while (0)

#define _test_m8abs_imm(addr, imm) \
do { OP1(0xf6); MODRM_MABS(0, addr); OP1(imm); } while (0)

#define _and_m16bd_r16(base, disp, sreg) \
do { OP1(0x66); OP1(0x21); MODRM_MBD(sreg, base, disp); } while (0)

#define _and_m8bd_r8(base, disp, sreg) \
do { OP1(0x20); MODRM_MBD(sreg, base, disp); } while (0)

#define _and_r16_m16abs(dreg, addr) \
do { OP1(0x66); OP1(0x25); MODRM_MABS(dreg, addr); } while (0)

#define _shl_r16_cl(dreg) \
do { OP1(0x66); OP1(0xd3); MODRM_REG(4, dreg); } while (0)

#define _shr_r16_cl(dreg) \
do { OP1(0x66); OP1(0xd3); MODRM_REG(5, dreg); } while (0)

#define _shl_r8_cl(dreg) \
do { OP1(0xd2); MODRM_REG(4, dreg); } while (0)

#define _shr_r8_cl(dreg) \
do { OP1(0xd2); MODRM_REG(5, dreg); } while (0)

#define _rol_r16_cl(reg) \
do { OP1(0x66); OP1(0xd3); OP1(0xc0 | ((reg) & 7)); } while(0)

#define _rol_r8_cl(reg) \
do { OP1(0xd2);	OP1(0xc0 | ((reg) & 7)); } while(0)

#define _ror_r16_cl(reg) \
do { OP1(0x66); OP1(0xd3); OP1(0xc8 | ((reg) & 7)); } while(0)

#define _ror_r8_cl(reg) \
do { OP1(0xd2);	OP1(0xc8 | ((reg) & 7)); } while(0)

#define _rcl_r16_cl(reg) \
do { OP1(0x66); OP1(0xd3); OP1(0xd0 | ((reg) & 7)); } while(0)

#define _rcl_r8_cl(reg) \
do { OP1(0xd2);	OP1(0xd0 | ((reg) & 7)); } while(0)

#define _rcr_r16_cl(reg) \
do { OP1(0x66); OP1(0xd3);	OP1(0xd8 | ((reg) & 7)); } while(0)

#define _rcr_r8_cl(reg) \
do { OP1(0xd2);	OP1(0xd8 | ((reg) & 7)); } while(0)


/*###################################################################################################
**	FLOATING POINT EMITTERS
**#################################################################################################*/

#define _fnclex() \
do { OP1(0xdb); OP1(0xe2); } while (0)

#define _fnstsw_ax() \
do { OP1(0xdf); OP1(0xe0); } while (0)

#define _fldcw_m16abs(addr) \
do { OP1(0xd9); MODRM_MABS(5, addr); } while (0)

#define _fldcw_m16isd(indx, scale, addr) \
do { OP1(0xd9); MODRM_MBISD(5, NO_BASE, indx, scale, addr); } while (0)

#define _fnstcw_m16abs(addr) \
do { OP1(0xd9); MODRM_MABS(7, addr); } while (0)



#define _fabs() \
do { OP1(0xd9); OP1(0xe1); } while (0)

#define _faddp() \
do { OP1(0xde); OP1(0xc1); } while (0)

#define _fchs() \
do { OP1(0xd9); OP1(0xe0); } while (0)

#define _fcompp() \
do { OP1(0xde); OP1(0xd9); } while (0)

#define _fdivp() \
do { OP1(0xde); OP1(0xf9); } while (0)

#define _fmulp() \
do { OP1(0xde); OP1(0xc9); } while (0)

#define _fsqrt() \
do { OP1(0xd9); OP1(0xfa); } while (0)

#define _fsubp() \
do { OP1(0xde); OP1(0xe9); } while (0)

#define _fsubrp() \
do { OP1(0xde); OP1(0xe1); } while (0)

#define _fucompp() \
do { OP1(0xda); OP1(0xe9); } while (0)



#define _fld1() \
do { OP1(0xd9); OP1(0xe8); } while (0)

#define _fld_m32abs(addr) \
do { OP1(0xd9); MODRM_MABS(0, addr); } while (0)

#define _fld_m64abs(addr) \
do { OP1(0xdd); MODRM_MABS(0, addr); } while (0)

#define _fild_m32abs(addr) \
do { OP1(0xdb); MODRM_MABS(0, addr); } while (0)

#define _fild_m64abs(addr) \
do { OP1(0xdf); MODRM_MABS(5, addr); } while (0)



#define _fistp_m32abs(addr) \
do { OP1(0xdb); MODRM_MABS(3, addr); } while (0)

#define _fistp_m64abs(addr) \
do { OP1(0xdf); MODRM_MABS(7, addr); } while (0)

#define _fstp_m32abs(addr) \
do { OP1(0xd9); MODRM_MABS(3, addr); } while (0)

#define _fstp_m64abs(addr) \
do { OP1(0xdd); MODRM_MABS(3, addr); } while (0)



/*###################################################################################################
**	BRANCH EMITTERS
**#################################################################################################*/

#define _setcc_r8(cond, dreg) \
do { OP1(0x0f); OP1(0x90 + cond); MODRM_REG(0, dreg); } while (0)

#define _setcc_m8abs(cond, addr) \
do { OP1(0x0f); OP1(0x90 + cond); MODRM_MABS(0, addr); } while (0)


#define _jcc_short_link(cond, link) \
do { OP1(0x70 + (cond)); OP1(0x00); (link)->target = drc->cache_top; (link)->size = 1; } while (0)

#define _jcc_near_link(cond, link) \
do { OP1(0x0f); OP1(0x80 + (cond)); OP4(0x00); (link)->target = drc->cache_top; (link)->size = 4; } while (0)

#define _jcc(cond, target) 									\
do {														\
	INT32 delta = (UINT8 *)(target) - (drc->cache_top + 2);	\
	if ((INT8)delta == (INT32)delta)						\
	{														\
		OP1(0x70 + (cond));	OP1(delta);						\
	}														\
	else													\
	{														\
		delta = (UINT8 *)(target) - (drc->cache_top + 6);	\
		OP1(0x0f); OP1(0x80 + (cond)); OP4(delta);			\
	}														\
} while (0)



#define _jmp_short_link(link) \
do { OP1(0xeb); OP1(0x00); (link)->target = drc->cache_top; (link)->size = 1; } while (0)

#define _jmp_near_link(link) \
do { OP1(0xe9); OP4(0x00); (link)->target = drc->cache_top; (link)->size = 4; } while (0)

#define _jmp(target) \
do { OP1(0xe9); OP4((UINT32)(target) - ((UINT32)drc->cache_top + 4)); } while (0)



#define _call(target) \
do { if (drc->uses_fp) OP1(0xe8); OP4((UINT32)(target) - ((UINT32)drc->cache_top + 4)); } while (0)



#define _jmp_m32abs(addr) \
do { OP1(0xff); MODRM_MABS(4, addr); } while (0)

#define _jmp_m32bd(base, disp) \
do { OP1(0xff); MODRM_MBD(4, base, disp); } while (0)

#define _jmp_m32bisd(base, indx, scale, disp) \
do { OP1(0xff); MODRM_MBISD(4, base, indx, scale, disp); } while (0)



#define _resolve_link(link)							\
do {												\
	INT32 delta = drc->cache_top - (link)->target;	\
	if ((link)->size == 1)							\
	{												\
		if ((INT8)delta != delta)					\
			printf("Error: link out of range!\n");	\
		(link)->target[-1] = delta;					\
	}												\
	else if ((link)->size == 4)						\
		*(UINT32 *)&(link)->target[-4] = delta;		\
	else											\
		printf("Unsized link!\n");					\
} while (0)



/*###################################################################################################
**	SSE EMITTERS
**#################################################################################################*/

#define _movsd_r128_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x10); MODRM_MABS(reg, addr); } while (0)

#define _movsd_m64abs_r128(addr, reg) \
do { OP1(0xf2); OP1(0x0f); OP1(0x11); MODRM_MABS(reg, addr); } while (0)

#define _movss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x10); MODRM_MABS(reg, addr); } while (0)

#define _movss_m32abs_r128(addr, reg) \
do { OP1(0xf3); OP1(0x0f); OP1(0x11); MODRM_MABS(reg, addr); } while (0)



#define _addss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x58); MODRM_MABS(reg, addr); } while (0)

#define _addsd_r128_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x58); MODRM_MABS(reg, addr); } while (0)

#define _comiss_r128_m32abs(reg, addr) \
do { OP1(0x0f); OP1(0x2f); MODRM_MABS(reg, addr); } while (0)

#define _comisd_r128_m64abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0x2f); MODRM_MABS(reg, addr); } while (0)

#define _divss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x5e); MODRM_MABS(reg, addr); } while (0)

#define _divsd_r128_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x5e); MODRM_MABS(reg, addr); } while (0)

#define _mulss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x59); MODRM_MABS(reg, addr); } while (0)

#define _mulsd_r128_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x59); MODRM_MABS(reg, addr); } while (0)

#define _sqrtss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x51); MODRM_MABS(reg, addr); } while (0)

#define _sqrtsd_r128_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x51); MODRM_MABS(reg, addr); } while (0)

#define _subss_r128_m32abs(reg, addr) \
do { OP1(0xf3); OP1(0x0f); OP1(0x5c); MODRM_MABS(reg, addr); } while (0)

#define _subsd_r128_m64abs(reg, addr) \
do { OP1(0xf2); OP1(0x0f); OP1(0x5c); MODRM_MABS(reg, addr); } while (0)

#define _ucomiss_r128_m32abs(reg, addr) \
do { OP1(0x0f); OP1(0x2e); MODRM_MABS(reg, addr); } while (0)

#define _ucomisd_r128_m64abs(reg, addr) \
do { OP1(0x66); OP1(0x0f); OP1(0x2e); MODRM_MABS(reg, addr); } while (0)



#define _subss_r128_r128(r1, r2) \
do { OP1(0xf3); OP1(0x0f); OP1(0x5c); MODRM_REG(r1, r2); } while (0)



#define _paddq_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xd4); MODRM_REG(r1, r2); } while (0)

#define _pand_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xdb); MODRM_REG(r1, r2); } while (0)

#define _pandn_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xdf); MODRM_REG(r1, r2); } while (0)

#define _por_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xeb); MODRM_REG(r1, r2); } while (0)

#define _psllq_r128_imm(reg, imm) \
do { OP1(0x66); OP1(0x0f); OP1(0x73); MODRM_REG(6, reg); OP1(imm); } while (0)

#define _psllq_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xf3); MODRM_REG(r1, r2); } while (0)

#define _psrlq_r128_imm(reg, imm) \
do { OP1(0x66); OP1(0x0f); OP1(0x73); MODRM_REG(2, reg); OP1(imm); } while (0)

#define _psrlq_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xd3); MODRM_REG(r1, r2); } while (0)

#define _psubq_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xfb); MODRM_REG(r1, r2); } while (0)

#define _pxor_r128_r128(r1, r2) \
do { OP1(0x66); OP1(0x0f); OP1(0xef); MODRM_REG(r1, r2); } while (0)



/*###################################################################################################
**	FUNCTION PROTOTYPES
**#################################################################################################*/

/* init/shutdown */
struct drccore *drc_init(UINT8 cpunum, struct drcconfig *config);
void drc_cache_reset(struct drccore *drc);
void drc_execute(struct drccore *drc);
void drc_exit(struct drccore *drc);

/* code management */
void drc_begin_sequence(struct drccore *drc, UINT32 pc);
void drc_end_sequence(struct drccore *drc);
void drc_register_code_at_cache_top(struct drccore *drc, UINT32 pc);
void *drc_get_code_at_pc(struct drccore *drc, UINT32 pc);

/* standard appendages */
void drc_append_dispatcher(struct drccore *drc);
void drc_append_fixed_dispatcher(struct drccore *drc, UINT32 newpc);
void drc_append_tentative_fixed_dispatcher(struct drccore *drc, UINT32 newpc);
void drc_append_call_debugger(struct drccore *drc);
void drc_append_standard_epilogue(struct drccore *drc, INT32 cycles, INT32 pcdelta, int allow_exit);
void drc_append_save_volatiles(struct drccore *drc);
void drc_append_restore_volatiles(struct drccore *drc);
void drc_append_save_call_restore(struct drccore *drc, void *target, UINT32 stackadj);
void drc_append_verify_code(struct drccore *drc, void *code, UINT8 length);
void drc_append_set_fp_rounding(struct drccore *drc, UINT8 regindex);
void drc_append_set_temp_fp_rounding(struct drccore *drc, UINT8 rounding);
void drc_append_restore_fp_rounding(struct drccore *drc);

/* disassembling drc code */
void drc_dasm(FILE *f, unsigned pc, void *begin, void *end);



#endif
