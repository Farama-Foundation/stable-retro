/*###################################################################################################
**
**	TMS34010: Portable Texas Instruments TMS34010 emulator
**
**	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998
**	 Parts based on code by Aaron Giles
**
**#################################################################################################*/


/*###################################################################################################
**	MISC MACROS
**#################################################################################################*/

#define ZEXTEND(val,width) if (width) (val) &= ((UINT32)0xffffffff >> (32 - (width)))
#define SEXTEND(val,width) if (width) (val) = (INT32)((val) << (32 - (width))) >> (32 - (width))

#define SXYTOL(val)	((((INT16)(val).y * state.convsp) + ((INT16)(val).x << state.pixelshift)) + OFFSET)
#define DXYTOL(val)	((((INT16)(val).y * state.convdp) + ((INT16)(val).x << state.pixelshift)) + OFFSET)
#define MXYTOL(val)	((((INT16)(val).y * state.convmp) + ((INT16)(val).x << state.pixelshift)) + OFFSET)

#define COUNT_CYCLES(x)	tms34010_ICount -= x
#define COUNT_UNKNOWN_CYCLES(x) COUNT_CYCLES(x)

#define CORRECT_ODD_PC(x)	do { if (PC & 0x0f) logerror("%s to PC=%08X\n", x, PC); PC &= ~0x0f; } while (0)



/*###################################################################################################
**	FLAG HANDLING MACROS
**#################################################################################################*/

#define SIGN(val)			((val) & 0x80000000)

#define CLR_V 				(V_FLAG = 0)

#define SET_Z(val)			(NOTZ_FLAG = (val))
#define SET_N(val)			(N_FLAG = SIGN(val))
#define SET_NZ(val)			{ SET_Z(val); SET_N(NOTZ_FLAG); }
#define SET_V_SUB(a,b,r)	(V_FLAG = SIGN(((a) ^ (b)) & ((a) ^ (r))))
#define SET_V_ADD(a,b,r)	(V_FLAG = SIGN(~((a) ^ (b)) & ((a) ^ (r))))
#define SET_C_SUB(a,b)		(C_FLAG = (((UINT32)(b)) > ((UINT32)(a))))
#define SET_C_ADD(a,b)		(C_FLAG = (((UINT32)(~(a))) < ((UINT32)(b))))
#define SET_NZV_SUB(a,b,r)	{ SET_NZ(r); SET_V_SUB(a,b,r); }
#define SET_NZCV_SUB(a,b,r)	{ SET_NZV_SUB(a,b,r); SET_C_SUB(a,b); }
#define SET_NZCV_ADD(a,b,r)	{ SET_NZ(r); SET_V_ADD(a,b,r); SET_C_ADD(a,b); }



/*###################################################################################################
**	UNIMPLEMENTED INSTRUCTION
**#################################################################################################*/

static void unimpl(void)
{
	/* kludge for Super High Impact -- this doesn't seem to cause */
	/* an illegal opcode exception */
	if (cpu_readop16(TOBYTE(PC - 0x10)) == 0x0007)
		return;

	/* 9 Ball Shootout calls to FFDF7468, expecting it */
	/* to execute the next instruction from FFDF7470 */
	/* but the instruction at FFDF7460 is an 0x0001 */
	if (cpu_readop16(TOBYTE(PC - 0x10)) == 0x0001)
		return;

	PUSH(PC);
	PUSH(GET_ST());
	RESET_ST();
	PC = RLONG(0xfffffc20);
	change_pc29lew(TOBYTE(PC));
  	COUNT_UNKNOWN_CYCLES(16);

	/* extra check to prevent bad things */
	if (PC == 0 || opcode_table[cpu_readop16(TOBYTE(PC)) >> 4] == unimpl)
	{
		cpu_set_halt_line(cpu_getactivecpu(),ASSERT_LINE);
#ifdef MAME_DEBUG
		debug_key_pressed = 1;
#endif
	}
}



/*###################################################################################################
**	X/Y OPERATIONS
**#################################################################################################*/

#define ADD_XY(R)								\
{												\
	XY res;										\
	XY  a =  R##REG_XY(R##SRCREG);				\
	XY *b = &R##REG_XY(R##DSTREG);				\
	res.x = b->x + a.x;							\
	   N_FLAG = !res.x;							\
	   V_FLAG = res.x & 0x8000;					\
	res.y = b->y + a.y;							\
	NOTZ_FLAG = res.y;							\
	   C_FLAG = res.y & 0x8000;					\
  	*b = res;									\
  	COUNT_CYCLES(1);							\
}
static void add_xy_a(void) { ADD_XY(A); }
static void add_xy_b(void) { ADD_XY(B); }

#define SUB_XY(R)								\
{												\
	XY  a =  R##REG_XY(R##SRCREG);				\
	XY *b = &R##REG_XY(R##DSTREG);				\
	   N_FLAG = (a.x == b->x);					\
	   V_FLAG = (a.x >  b->x);					\
	   C_FLAG = (a.y >  b->y);					\
	NOTZ_FLAG = (a.y != b->y);					\
	b->x -= a.x;								\
	b->y -= a.y;								\
  	COUNT_CYCLES(1);							\
}
static void sub_xy_a(void) { SUB_XY(A); }
static void sub_xy_b(void) { SUB_XY(B); }

#define CMP_XY(R)								\
{												\
	INT16 res;									\
	XY a = R##REG_XY(R##DSTREG);				\
	XY b = R##REG_XY(R##SRCREG);				\
	res = a.x-b.x;								\
	   N_FLAG = !res;							\
	   V_FLAG = (res & 0x8000);					\
	res = a.y-b.y;								\
	NOTZ_FLAG =  res;							\
	   C_FLAG = (res & 0x8000);					\
  	COUNT_CYCLES(1);							\
}
static void cmp_xy_a(void) { CMP_XY(A); }
static void cmp_xy_b(void) { CMP_XY(B); }

#define CPW(R)									\
{												\
	INT32 res = 0;								\
	INT16 x = R##REG_X(R##SRCREG);				\
	INT16 y = R##REG_Y(R##SRCREG);				\
												\
	res |= ((WSTART_X > x) ? 0x20  : 0);		\
	res |= ((x > WEND_X)   ? 0x40  : 0);		\
	res |= ((WSTART_Y > y) ? 0x80  : 0);		\
	res |= ((y > WEND_Y)   ? 0x100 : 0);		\
	R##REG(R##DSTREG) = V_FLAG = res;			\
  	COUNT_CYCLES(1);							\
}
static void cpw_a(void) { CPW(A); }
static void cpw_b(void) { CPW(B); }

#define CVXYL(R)									\
{													\
    R##REG(R##DSTREG) = DXYTOL(R##REG_XY(R##SRCREG));\
  	COUNT_CYCLES(3);								\
}
static void cvxyl_a(void) { CVXYL(A); }
static void cvxyl_b(void) { CVXYL(B); }

#define MOVX(R)										\
{													\
	R##REG(R##DSTREG) = (R##REG(R##DSTREG) & 0xffff0000) | (UINT16)R##REG(R##SRCREG);	\
  	COUNT_CYCLES(1);																	\
}
static void movx_a(void) { MOVX(A); }
static void movx_b(void) { MOVX(B); }

#define MOVY(R)										\
{													\
	R##REG(R##DSTREG) = (R##REG(R##SRCREG) & 0xffff0000) | (UINT16)R##REG(R##DSTREG);	\
  	COUNT_CYCLES(1);																	\
}
static void movy_a(void) { MOVY(A); }
static void movy_b(void) { MOVY(B); }



/*###################################################################################################
**	PIXEL TRANSFER OPERATIONS
**#################################################################################################*/

#define PIXT_RI(R)			                        \
{							 						\
	WPIXEL(R##REG(R##DSTREG),R##REG(R##SRCREG));	\
  	COUNT_UNKNOWN_CYCLES(2);						\
}
static void pixt_ri_a(void) { PIXT_RI(A); }
static void pixt_ri_b(void) { PIXT_RI(B); }

#define PIXT_RIXY(R)		                       									\
{																					\
	if (state.window_checking != 0)													\
	{																				\
		if (R##REG_X(R##DSTREG) < WSTART_X || R##REG_X(R##DSTREG) > WEND_X ||		\
			R##REG_Y(R##DSTREG) < WSTART_Y || R##REG_Y(R##DSTREG) > WEND_Y)			\
		{																			\
			V_FLAG = 1;																\
			goto skip;																\
		}																			\
		V_FLAG = 0;																	\
		if (state.window_checking == 1) goto skip;									\
	}																				\
	WPIXEL(DXYTOL(R##REG_XY(R##DSTREG)),R##REG(R##SRCREG));							\
skip: 																				\
  	COUNT_UNKNOWN_CYCLES(4);														\
}
static void pixt_rixy_a(void) { PIXT_RIXY(A); }
static void pixt_rixy_b(void) { PIXT_RIXY(B); }

#define PIXT_IR(R)			                        \
{													\
	R##REG(R##DSTREG) = V_FLAG = RPIXEL(R##REG(R##SRCREG));	\
	COUNT_CYCLES(4);								\
}
static void pixt_ir_a(void) { PIXT_IR(A); }
static void pixt_ir_b(void) { PIXT_IR(B); }

#define PIXT_II(R)			                       	\
{													\
	WPIXEL(R##REG(R##DSTREG),RPIXEL(R##REG(R##SRCREG)));	\
  	COUNT_UNKNOWN_CYCLES(4);						\
}
static void pixt_ii_a(void) { PIXT_II(A); }
static void pixt_ii_b(void) { PIXT_II(B); }

#define PIXT_IXYR(R)			              		\
{													\
	R##REG(R##DSTREG) = V_FLAG = RPIXEL(SXYTOL(R##REG_XY(R##SRCREG)));	\
	COUNT_CYCLES(6);								\
}
static void pixt_ixyr_a(void) { PIXT_IXYR(A); }
static void pixt_ixyr_b(void) { PIXT_IXYR(B); }

#define PIXT_IXYIXY(R)			              			      						\
{																					\
	if (state.window_checking != 0)													\
	{																				\
		if (R##REG_X(R##DSTREG) < WSTART_X || R##REG_X(R##DSTREG) > WEND_X ||		\
			R##REG_Y(R##DSTREG) < WSTART_Y || R##REG_Y(R##DSTREG) > WEND_Y)			\
		{																			\
			V_FLAG = 1;																\
			goto skip;																\
		}																			\
		V_FLAG = 0;																	\
		if (state.window_checking == 1) goto skip;									\
	}																				\
	WPIXEL(DXYTOL(R##REG_XY(R##DSTREG)),RPIXEL(SXYTOL(R##REG_XY(R##SRCREG))));		\
skip: 																				\
  	COUNT_UNKNOWN_CYCLES(7);														\
}
static void pixt_ixyixy_a(void) { PIXT_IXYIXY(A); }
static void pixt_ixyixy_b(void) { PIXT_IXYIXY(B); }

#define DRAV(R)			              			      								\
{																					\
	if (state.window_checking != 0)													\
	{																				\
		if (R##REG_X(R##DSTREG) < WSTART_X || R##REG_X(R##DSTREG) > WEND_X ||		\
			R##REG_Y(R##DSTREG) < WSTART_Y || R##REG_Y(R##DSTREG) > WEND_Y)			\
		{																			\
			V_FLAG = 1;																\
			goto skip;																\
		}																			\
		V_FLAG = 0;																	\
		if (state.window_checking == 1) goto skip;									\
	}																				\
	WPIXEL(DXYTOL(R##REG_XY(R##DSTREG)),COLOR1);									\
skip: 																				\
	R##REG_X(R##DSTREG) += R##REG_X(R##SRCREG);										\
	R##REG_Y(R##DSTREG) += R##REG_Y(R##SRCREG);										\
  	COUNT_UNKNOWN_CYCLES(4);														\
}
static void drav_a(void) { DRAV(A); }
static void drav_b(void) { DRAV(B); }



/*###################################################################################################
**	ARITHMETIC OPERATIONS
**#################################################################################################*/

#define ABS(R)			              			      		\
{															\
	INT32 *rd = &R##REG(R##DSTREG);							\
	INT32 r = 0 - *rd;										\
	SET_NZV_SUB(0,*rd,r);									\
	if (!N_FLAG)											\
	{														\
		*rd = r;											\
	}														\
	COUNT_CYCLES(1);										\
}
static void abs_a(void) { ABS(A); }
static void abs_b(void) { ABS(B); }

#define ADD(R)			              			      		\
{							 								\
	INT32 a = R##REG(R##SRCREG);							\
	INT32 *rd = &R##REG(R##DSTREG);							\
	INT32 b = *rd;											\
	INT32 r = *rd = a + b;									\
	SET_NZCV_ADD(a,b,r);									\
	COUNT_CYCLES(1);										\
}
static void add_a(void) { ADD(A); }
static void add_b(void) { ADD(B); }

#define ADDC(R)			              			      		\
{			  												\
	/* I'm not sure to which side the carry is added to, should	*/	\
	/* verify it against the examples */					\
	INT32 a = R##REG(R##SRCREG);							\
	INT32 *rd = &R##REG(R##DSTREG);							\
	INT32 b = *rd;											\
	INT32 r = *rd = a + b + (C_FLAG?1:0);					\
	SET_NZCV_ADD(a,b,r);									\
	COUNT_CYCLES(1);										\
}
static void addc_a(void) { ADDC(A); }
static void addc_b(void) { ADDC(B); }

#define ADDI_W(R)			              			      	\
{			  												\
	INT32 a = PARAM_WORD();									\
	INT32 *rd = &R##REG(R##DSTREG);							\
	INT32 b = *rd;											\
	INT32 r = *rd = a + b;									\
	SET_NZCV_ADD(a,b,r);									\
	COUNT_CYCLES(2);										\
}
static void addi_w_a(void) { ADDI_W(A); }
static void addi_w_b(void) { ADDI_W(B); }

#define ADDI_L(R)			              			      	\
{			  												\
	INT32 a = PARAM_LONG();									\
	INT32 *rd = &R##REG(R##DSTREG);							\
	INT32 b = *rd;											\
	INT32 r = *rd = a + b;									\
	SET_NZCV_ADD(a,b,r);									\
	COUNT_CYCLES(3);										\
}
static void addi_l_a(void) { ADDI_L(A); }
static void addi_l_b(void) { ADDI_L(B); }

#define ADDK(R)				              			      	\
{			  												\
	INT32 r,b,*rd;											\
	INT32 a = PARAM_K; if (!a) a = 32;						\
	rd = &R##REG(R##DSTREG);								\
	b = *rd;												\
	r = *rd = a + b;										\
	SET_NZCV_ADD(a,b,r);									\
	COUNT_CYCLES(1);										\
}
static void addk_a(void) { ADDK(A); }
static void addk_b(void) { ADDK(B); }

#define AND(R)				              			      	\
{			  												\
	INT32 *rd = &R##REG(R##DSTREG);							\
	*rd &= R##REG(R##SRCREG);								\
	SET_Z(*rd);												\
	COUNT_CYCLES(1);										\
}
static void and_a(void) { AND(A); }
static void and_b(void) { AND(B); }

#define ANDI(R)				              			      	\
{			  												\
	INT32 *rd = &R##REG(R##DSTREG);							\
	*rd &= ~PARAM_LONG();									\
	SET_Z(*rd);												\
	COUNT_CYCLES(3);										\
}
static void andi_a(void) { ANDI(A); }
static void andi_b(void) { ANDI(B); }

#define ANDN(R)				              			      	\
{			  												\
	INT32 *rd = &R##REG(R##DSTREG);							\
	*rd &= ~R##REG(R##SRCREG);								\
	SET_Z(*rd);												\
	COUNT_CYCLES(1);										\
}
static void andn_a(void) { ANDN(A); }
static void andn_b(void) { ANDN(B); }

#define BTST_K(R)				              			    \
{							 								\
	SET_Z(R##REG(R##DSTREG) & (1<<(31-PARAM_K)));			\
	COUNT_CYCLES(1);										\
}
static void btst_k_a(void) { BTST_K(A); }
static void btst_k_b(void) { BTST_K(B); }

#define BTST_R(R)				              			    \
{															\
	SET_Z(R##REG(R##DSTREG) & (1<<(R##REG(R##SRCREG)&0x1f)));	\
	COUNT_CYCLES(2);										\
}
static void btst_r_a(void) { BTST_R(A); }
static void btst_r_b(void) { BTST_R(B); }

static void clrc(void)
{
	C_FLAG = 0;
	COUNT_CYCLES(1);
}

#define CMP(R)				       		       			    \
{															\
	INT32 *rs = &R##REG(R##SRCREG);							\
	INT32 *rd = &R##REG(R##DSTREG);							\
	INT32 r = *rd - *rs;									\
	SET_NZCV_SUB(*rd,*rs,r);								\
	COUNT_CYCLES(1);										\
}
static void cmp_a(void) { CMP(A); }
static void cmp_b(void) { CMP(B); }

#define CMPI_W(R)			       		       			    \
{															\
	INT32 r;												\
	INT32 *rd = &R##REG(R##DSTREG);							\
	INT32 t = ~PARAM_WORD();								\
	r = *rd - t;											\
	SET_NZCV_SUB(*rd,t,r);									\
	COUNT_CYCLES(2);										\
}
static void cmpi_w_a(void) { CMPI_W(A); }
static void cmpi_w_b(void) { CMPI_W(B); }

#define CMPI_L(R)			       		       			    \
{															\
	INT32 *rd = &R##REG(R##DSTREG);							\
	INT32 t = ~PARAM_LONG();								\
	INT32 r = *rd - t;										\
	SET_NZCV_SUB(*rd,t,r);									\
	COUNT_CYCLES(3);										\
}
static void cmpi_l_a(void) { CMPI_L(A); }
static void cmpi_l_b(void) { CMPI_L(B); }

static void dint(void)
{
	IE_FLAG = 0;
	COUNT_CYCLES(3);
}

#define DIVS(R,N)			       		       			    \
{															\
	INT32 *rs  = &R##REG(R##SRCREG);						\
	INT32 *rd1 = &R##REG(R##DSTREG);						\
	V_FLAG = N_FLAG = 0;									\
	NOTZ_FLAG = 1;											\
	if (!(R##DSTREG & (1*N)))								\
	{														\
		if (!*rs)											\
		{													\
			V_FLAG = 1;										\
		}													\
		else												\
		{													\
			INT32 *rd2 = &R##REG(R##DSTREG+N);				\
			INT64 dividend  = COMBINE_64_32_32(*rd1, *rd2); \
			INT64 quotient  = DIV_64_64_32(dividend, *rs); 	\
			INT32 remainder = MOD_32_64_32(dividend, *rs); 	\
			UINT32 signbits = (INT32)quotient >> 31;	 	\
			if (HI32_32_64(quotient) != signbits)			\
			{												\
				V_FLAG = 1;									\
			}												\
			else											\
			{												\
				*rd1 = quotient;							\
				*rd2 = remainder;							\
				SET_NZ(*rd1);								\
			}												\
		}													\
		COUNT_CYCLES(40);									\
	}														\
	else													\
	{														\
		if (!*rs)											\
		{													\
			V_FLAG = 1;										\
		}													\
		else												\
		{													\
			*rd1 /= *rs;									\
			SET_NZ(*rd1);									\
		}													\
		COUNT_CYCLES(39);									\
	}														\
}
static void divs_a(void) { DIVS(A,1   ); }
static void divs_b(void) { DIVS(B,0x10); }

#define DIVU(R,N)			       		       			    \
{										  					\
	INT32 *rs  = &R##REG(R##SRCREG);						\
	INT32 *rd1 = &R##REG(R##DSTREG);						\
	V_FLAG = 0;												\
	NOTZ_FLAG = 1;											\
	if (!(R##DSTREG & (1*N)))								\
	{														\
		if (!*rs)											\
		{													\
			V_FLAG = 1;										\
		}													\
		else												\
		{													\
			INT32 *rd2 = &R##REG(R##DSTREG+N);				\
			UINT64 dividend  = COMBINE_U64_U32_U32(*rd1, *rd2);	\
			UINT64 quotient  = DIV_U64_U64_U32(dividend, *rs);	\
			UINT32 remainder = MOD_U32_U64_U32(dividend, *rs); 	\
			if (HI32_U32_U64(quotient) != 0)				\
			{												\
				V_FLAG = 1;									\
			}												\
			else											\
			{												\
				*rd1 = quotient;							\
				*rd2 = remainder;							\
				SET_Z(*rd1);								\
			}												\
		}													\
	}														\
	else													\
	{														\
		if (!*rs)											\
		{													\
			V_FLAG = 1;										\
		}													\
		else												\
		{													\
			*rd1 = (UINT32)*rd1 / (UINT32)*rs;			  	\
			SET_Z(*rd1);									\
		}													\
	}														\
	COUNT_CYCLES(37);										\
}
static void divu_a(void) { DIVU(A,1   ); }
static void divu_b(void) { DIVU(B,0x10); }

static void eint(void)
{
	IE_FLAG = 1;
	check_interrupt();
	COUNT_CYCLES(3);
}

#define EXGF(F,R)			       		       			    	\
{																\
	INT32 *rd = &R##REG(R##DSTREG);								\
	UINT32 temp = (FE##F##_FLAG ? 0x20 : 0) | FW(F);			\
	FE##F##_FLAG = (*rd&0x20);									\
	FW(F) = (*rd&0x1f);											\
	SET_FW();													\
	*rd = temp;													\
	COUNT_CYCLES(1);											\
}
static void exgf0_a(void) { EXGF(0,A); }
static void exgf0_b(void) { EXGF(0,B); }
static void exgf1_a(void) { EXGF(1,A); }
static void exgf1_b(void) { EXGF(1,B); }

#define LMO(R)			       		       			    		\
{																\
	UINT32 res = 0;												\
	UINT32 rs  = R##REG(R##SRCREG);								\
	 INT32 *rd = &R##REG(R##DSTREG);							\
	SET_Z(rs);													\
	if (rs)														\
	{															\
		while (!(rs & 0x80000000))								\
		{														\
			res++;												\
			rs <<= 1;											\
		}														\
	}															\
	*rd = res;													\
	COUNT_CYCLES(1);											\
}
static void lmo_a(void) { LMO(A); }
static void lmo_b(void) { LMO(B); }

#define MMFM(R,N)			       		       			    	\
{																\
	INT32 i;													\
	UINT16 l = (UINT16) PARAM_WORD();							\
	COUNT_CYCLES(3);											\
	{															\
		INT32 rd = R##DSTREG;									\
		for (i = 15; i >= 0 ; i--)								\
		{														\
			if (l & 0x8000)										\
			{													\
				R##REG(i*N) = RLONG(R##REG(rd));				\
				R##REG(rd) += 0x20;								\
				COUNT_CYCLES(4);								\
			}													\
			l <<= 1;											\
		}														\
	}															\
}
static void mmfm_a(void) { MMFM(A,1   ); }
static void mmfm_b(void) { MMFM(B,0x10); }

#define MMTM(R,N)			       		       			    	\
{			  													\
	UINT32 i;													\
	UINT16 l = (UINT16) PARAM_WORD();							\
	COUNT_CYCLES(2);											\
	{															\
		INT32 rd = R##DSTREG;									\
		SET_N(R##REG(rd)^0x80000000);							\
		for (i = 0; i  < 16; i++)								\
		{														\
			if (l & 0x8000)										\
			{													\
				R##REG(rd) -= 0x20;								\
				WLONG(R##REG(rd),R##REG(i*N));					\
				COUNT_CYCLES(4);								\
			}													\
			l <<= 1;											\
		}														\
	}															\
}
static void mmtm_a(void) { MMTM(A,1   ); }
static void mmtm_b(void) { MMTM(B,0x10); }

#define MODS(R)			       		       			    		\
{				  												\
	INT32 *rs = &R##REG(R##SRCREG);								\
	INT32 *rd = &R##REG(R##DSTREG);								\
	V_FLAG = (*rs == 0);										\
	if (!V_FLAG)												\
	{															\
		*rd %= *rs;												\
		SET_Z(*rd);												\
	}															\
	COUNT_CYCLES(40);											\
}
static void mods_a(void) { MODS(A); }
static void mods_b(void) { MODS(B); }

#define MODU(R)			       		       			    		\
{				  												\
	INT32 *rs = &R##REG(R##SRCREG);								\
	INT32 *rd = &R##REG(R##DSTREG);								\
	V_FLAG = (*rs == 0);										\
	if (!V_FLAG)												\
	{															\
		*rd = (UINT32)*rd % (UINT32)*rs;						\
		SET_Z(*rd);												\
	}															\
	COUNT_CYCLES(35);											\
}
static void modu_a(void) { MODU(A); }
static void modu_b(void) { MODU(B); }

#define MPYS(R,N)			       		       			    	\
{																\
	INT32 *rd1 = &R##REG(R##DSTREG);							\
																\
	INT32 m1 = R##REG(R##SRCREG);								\
	SEXTEND(m1, FW_INC(1));										\
																\
	if (!(R##DSTREG & (1*N)))									\
	{															\
		INT64 product    = MUL_64_32_32(m1, *rd1);				\
		*rd1             = HI32_32_64(product);					\
		R##REG(R##DSTREG+N) = LO32_32_64(product);				\
		SET_Z(product!=0);										\
		SET_N(*rd1);											\
	}															\
	else														\
	{															\
		INT64 product    = MUL_64_32_32(m1, *rd1);				\
		*rd1             = LO32_32_64(product);					\
		SET_Z(product!=0);										\
		SET_N(HI32_32_64(product));								\
	}															\
	COUNT_CYCLES(20);											\
}
static void mpys_a(void) { MPYS(A,1   ); }
static void mpys_b(void) { MPYS(B,0x10); }

#define MPYU(R,N)			       		       			    	\
{				  												\
	INT32 *rd1 = &R##REG(R##DSTREG);							\
																\
	UINT32 m1 = R##REG(R##SRCREG);								\
	ZEXTEND(m1, FW_INC(1));										\
																\
	if (!(R##DSTREG & (1*N)))									\
	{															\
		UINT64 product   = MUL_U64_U32_U32(m1, *rd1);			\
		*rd1             = HI32_U32_U64(product);				\
		R##REG(R##DSTREG+N) = LO32_U32_U64(product);			\
		SET_Z(product!=0);										\
	}															\
	else														\
	{															\
		UINT64 product   = MUL_U64_U32_U32(m1, *rd1);			\
		*rd1             = LO32_U32_U64(product);				\
		SET_Z(product!=0);										\
	}															\
	COUNT_CYCLES(21);											\
}
static void mpyu_a(void) { MPYU(A,1   ); }
static void mpyu_b(void) { MPYU(B,0x10); }

#define NEG(R)			       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 r = 0 - *rd;											\
	SET_NZCV_SUB(0,*rd,r);										\
	*rd = r;													\
	COUNT_CYCLES(1);											\
}
static void neg_a(void) { NEG(A); }
static void neg_b(void) { NEG(B); }

#define NEGB(R)			       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 t = *rd + (C_FLAG?1:0);								\
	INT32 r = 0 - t;											\
	SET_NZCV_SUB(0,t,r);										\
	*rd = r;													\
	COUNT_CYCLES(1);											\
}
static void negb_a(void) { NEGB(A); }
static void negb_b(void) { NEGB(B); }

static void nop(void)
{
	COUNT_CYCLES(1);
}

#define NOT(R)			       		       			    		\
{								 								\
	INT32 *rd = &R##REG(R##DSTREG);								\
	*rd = ~(*rd);												\
	SET_Z(*rd);													\
	COUNT_CYCLES(1);											\
}
static void not_a(void) { NOT(A); }
static void not_b(void) { NOT(B); }

#define OR(R)			       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	*rd |= R##REG(R##SRCREG);									\
	SET_Z(*rd);													\
	COUNT_CYCLES(1);											\
}
static void or_a(void) { OR(A); }
static void or_b(void) { OR(B); }

#define ORI(R)			       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	*rd |= PARAM_LONG();										\
	SET_Z(*rd);													\
	COUNT_CYCLES(3);											\
}
static void ori_a(void) { ORI(A); }
static void ori_b(void) { ORI(B); }

static void setc(void)
{
	C_FLAG = 1;
	COUNT_CYCLES(1);
}

#define SETF(F)													\
{																\
	FE##F##_FLAG = state.op & 0x20;								\
	FW(F) = state.op & 0x1f;									\
	SET_FW();													\
	COUNT_CYCLES(1+F);											\
}
static void setf0(void) { SETF(0); }
static void setf1(void) { SETF(1); }

#define SEXT(F,R)												\
{							   									\
	INT32 *rd = &R##REG(R##DSTREG);								\
	SEXTEND(*rd,FW_INC(F));										\
	SET_NZ(*rd);												\
	COUNT_CYCLES(3);											\
}
static void sext0_a(void) { SEXT(0,A); }
static void sext0_b(void) { SEXT(0,B); }
static void sext1_a(void) { SEXT(1,A); }
static void sext1_b(void) { SEXT(1,B); }

#define RL(R,K)			       		       			    		\
{			 													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 res = *rd;											\
	INT32 k = (K);												\
	if (k)														\
	{															\
		res<<=(k-1);											\
		C_FLAG = SIGN(res);										\
		res<<=1;												\
		res |= (((UINT32)*rd)>>((-k)&0x1f));					\
		*rd = res;												\
	}															\
	else														\
	{															\
		C_FLAG = 0;												\
	}															\
	SET_Z(res);													\
	COUNT_CYCLES(1);											\
}
static void rl_k_a(void) { RL(A,PARAM_K); }
static void rl_k_b(void) { RL(B,PARAM_K); }
static void rl_r_a(void) { RL(A,AREG(ASRCREG)&0x1f); }
static void rl_r_b(void) { RL(B,BREG(BSRCREG)&0x1f); }

#define SLA(R,K)												\
{				 												\
	 INT32 *rd = &R##REG(R##DSTREG);							\
	UINT32 res = *rd;											\
	 INT32 k = K;												\
	if (k)														\
	{															\
		UINT32 mask = (0xffffffff<<(31-k))&0x7fffffff;			\
		UINT32 res2 = SIGN(res) ? res^mask : res;				\
		V_FLAG = (res2 & mask);									\
																\
		res<<=(k-1);											\
		C_FLAG = SIGN(res);										\
		res<<=1;												\
		*rd = res;												\
	}															\
	else														\
	{															\
		C_FLAG = V_FLAG = 0;									\
	}															\
	SET_NZ(res);												\
	COUNT_CYCLES(3);											\
}
static void sla_k_a(void) { SLA(A,PARAM_K); }
static void sla_k_b(void) { SLA(B,PARAM_K); }
static void sla_r_a(void) { SLA(A,AREG(ASRCREG)&0x1f); }
static void sla_r_b(void) { SLA(B,BREG(BSRCREG)&0x1f); }

#define SLL(R,K)												\
{			 													\
	 INT32 *rd = &R##REG(R##DSTREG);							\
	UINT32 res = *rd;											\
	 INT32 k = K;												\
	if (k)														\
	{															\
		res<<=(k-1);											\
		C_FLAG = SIGN(res);										\
		res<<=1;												\
		*rd = res;												\
	}															\
	else														\
	{															\
		C_FLAG = 0;												\
	}															\
	SET_Z(res);													\
	COUNT_CYCLES(1);											\
}
static void sll_k_a(void) { SLL(A,PARAM_K); }
static void sll_k_b(void) { SLL(B,PARAM_K); }
static void sll_r_a(void) { SLL(A,AREG(ASRCREG)&0x1f); }
static void sll_r_b(void) { SLL(B,BREG(BSRCREG)&0x1f); }

#define SRA(R,K)												\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 res = *rd;											\
	INT32 k = (-(K)) & 0x1f;									\
	if (k)														\
	{															\
		res>>=(k-1);											\
		C_FLAG = res & 1;										\
		res>>=1;												\
		*rd = res;												\
	}															\
	else														\
	{															\
		C_FLAG = 0;												\
	}															\
	SET_NZ(res);												\
	COUNT_CYCLES(1);											\
}
static void sra_k_a(void) { SRA(A,PARAM_K); }
static void sra_k_b(void) { SRA(B,PARAM_K); }
static void sra_r_a(void) { SRA(A,AREG(ASRCREG)); }
static void sra_r_b(void) { SRA(B,BREG(BSRCREG)); }

#define SRL(R,K)												\
{			  													\
	 INT32 *rd = &R##REG(R##DSTREG);							\
	UINT32 res = *rd;											\
	 INT32 k = (-(K)) & 0x1f;									\
	if (k)														\
	{															\
		res>>=(k-1);											\
		C_FLAG = res & 1;										\
		res>>=1;												\
		*rd = res;												\
	}															\
	else														\
	{															\
		C_FLAG = 0;												\
	}															\
	SET_NZ(res);												\
	COUNT_CYCLES(1);											\
}
static void srl_k_a(void) { SRL(A,PARAM_K); }
static void srl_k_b(void) { SRL(B,PARAM_K); }
static void srl_r_a(void) { SRL(A,AREG(ASRCREG)); }
static void srl_r_b(void) { SRL(B,BREG(BSRCREG)); }

#define SUB(R)			       		       			    		\
{			  													\
	INT32 *rs = &R##REG(R##SRCREG);								\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 r = *rd - *rs;										\
	SET_NZCV_SUB(*rd,*rs,r);									\
	*rd = r;													\
	COUNT_CYCLES(1);											\
}
static void sub_a(void) { SUB(A); }
static void sub_b(void) { SUB(B); }

#define SUBB(R)			       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 t = R##REG(R##SRCREG);								\
	INT32 r = *rd - t - (C_FLAG?1:0);							\
	SET_NZCV_SUB(*rd,t,r);										\
	*rd = r;													\
	COUNT_CYCLES(1);											\
}
static void subb_a(void) { SUBB(A); }
static void subb_b(void) { SUBB(B); }

#define SUBI_W(R)			       		       			    	\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 r;													\
	INT32 t = ~PARAM_WORD();									\
	r = *rd - t;												\
	SET_NZCV_SUB(*rd,t,r);										\
	*rd = r;													\
	COUNT_CYCLES(2);											\
}
static void subi_w_a(void) { SUBI_W(A); }
static void subi_w_b(void) { SUBI_W(B); }

#define SUBI_L(R)			       		       			    	\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 t = ~PARAM_LONG();									\
	INT32 r = *rd - t;											\
	SET_NZCV_SUB(*rd,t,r);										\
	*rd = r;													\
	COUNT_CYCLES(3);											\
}
static void subi_l_a(void) { SUBI_L(A); }
static void subi_l_b(void) { SUBI_L(B); }

#define SUBK(R)			       		       			    		\
{			  													\
	INT32 r;													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 t = PARAM_K; if (!t) t = 32;							\
	r = *rd - t;												\
	SET_NZCV_SUB(*rd,t,r);										\
	*rd = r;													\
	COUNT_CYCLES(1);											\
}
static void subk_a(void) { SUBK(A); }
static void subk_b(void) { SUBK(B); }

#define XOR(R)			       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	*rd ^= R##REG(R##SRCREG);									\
	SET_Z(*rd);													\
	COUNT_CYCLES(1);											\
}
static void xor_a(void) { XOR(A); }
static void xor_b(void) { XOR(B); }

#define XORI(R)			       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	*rd ^= PARAM_LONG();										\
	SET_Z(*rd);													\
	COUNT_CYCLES(3);											\
}
static void xori_a(void) { XORI(A); }
static void xori_b(void) { XORI(B); }

#define ZEXT(F,R)												\
{																\
	INT32 *rd = &R##REG(R##DSTREG);								\
	ZEXTEND(*rd,FW_INC(F));										\
	SET_Z(*rd);													\
	COUNT_CYCLES(1);											\
}
static void zext0_a(void) { ZEXT(0,A); }
static void zext0_b(void) { ZEXT(0,B); }
static void zext1_a(void) { ZEXT(1,A); }
static void zext1_b(void) { ZEXT(1,B); }



/*###################################################################################################
**	MOVE INSTRUCTIONS
**#################################################################################################*/

#define MOVI_W(R)		       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	*rd=PARAM_WORD();											\
	SET_NZ(*rd);												\
	CLR_V;														\
	COUNT_CYCLES(2);											\
}
static void movi_w_a(void) { MOVI_W(A); }
static void movi_w_b(void) { MOVI_W(B); }

#define MOVI_L(R)		       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	*rd=PARAM_LONG();											\
	SET_NZ(*rd);												\
	CLR_V;														\
	COUNT_CYCLES(3);											\
}
static void movi_l_a(void) { MOVI_L(A); }
static void movi_l_b(void) { MOVI_L(B); }

#define MOVK(R)		       		       			    			\
{																\
	INT32 k = PARAM_K; if (!k) k = 32;							\
	R##REG(R##DSTREG) = k;										\
	COUNT_CYCLES(1);											\
}
static void movk_a(void) { MOVK(A); }
static void movk_b(void) { MOVK(B); }

#define MOVB_RN(R)		       		       			    		\
{																\
	WBYTE(R##REG(R##DSTREG),R##REG(R##SRCREG));					\
	COUNT_CYCLES(1);											\
}
static void movb_rn_a(void) { MOVB_RN(A); }
static void movb_rn_b(void) { MOVB_RN(B); }

#define MOVB_NR(R)		       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	*rd = (INT8)RBYTE(R##REG(R##SRCREG));						\
	SET_NZ(*rd);												\
	CLR_V;														\
	COUNT_CYCLES(3);											\
}
static void movb_nr_a(void) { MOVB_NR(A); }
static void movb_nr_b(void) { MOVB_NR(B); }

#define MOVB_NN(R)												\
{																\
	WBYTE(R##REG(R##DSTREG),(UINT32)(UINT8)RBYTE(R##REG(R##SRCREG)));	\
	COUNT_CYCLES(3);											\
}
static void movb_nn_a(void) { MOVB_NN(A); }
static void movb_nn_b(void) { MOVB_NN(B); }

#define MOVB_R_NO(R)	       		       			    		\
{							  									\
	INT32 o = PARAM_WORD();										\
	WBYTE(R##REG(R##DSTREG)+o,R##REG(R##SRCREG));				\
	COUNT_CYCLES(3);											\
}
static void movb_r_no_a(void) { MOVB_R_NO(A); }
static void movb_r_no_b(void) { MOVB_R_NO(B); }

#define MOVB_NO_R(R)	       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 o = PARAM_WORD();										\
	*rd = (INT8)RBYTE(R##REG(R##SRCREG)+o);						\
	SET_NZ(*rd);												\
	CLR_V;														\
	COUNT_CYCLES(5);											\
}
static void movb_no_r_a(void) { MOVB_NO_R(A); }
static void movb_no_r_b(void) { MOVB_NO_R(B); }

#define MOVB_NO_NO(R)	       		       			    		\
{																\
	INT32 o1 = PARAM_WORD();									\
	INT32 o2 = PARAM_WORD();									\
	WBYTE(R##REG(R##DSTREG)+o2,(UINT32)(UINT8)RBYTE(R##REG(R##SRCREG)+o1));	\
	COUNT_CYCLES(5);											\
}
static void movb_no_no_a(void) { MOVB_NO_NO(A); }
static void movb_no_no_b(void) { MOVB_NO_NO(B); }

#define MOVB_RA(R)	       		       			    			\
{																\
	WBYTE(PARAM_LONG(),R##REG(R##DSTREG));						\
	COUNT_CYCLES(1);											\
}
static void movb_ra_a(void) { MOVB_RA(A); }
static void movb_ra_b(void) { MOVB_RA(B); }

#define MOVB_AR(R)	       		       			    			\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	*rd = (INT8)RBYTE(PARAM_LONG());							\
	SET_NZ(*rd);												\
	CLR_V;														\
	COUNT_CYCLES(5);											\
}
static void movb_ar_a(void) { MOVB_AR(A); }
static void movb_ar_b(void) { MOVB_AR(B); }

static void movb_aa(void)
{
	UINT32 bitaddrs=PARAM_LONG();
	WBYTE(PARAM_LONG(),(UINT32)(UINT8)RBYTE(bitaddrs));
	COUNT_CYCLES(6);
}

#define MOVE_RR(RS,RD)	       		       			    		\
{																\
	INT32 *rd = &RD##REG(RD##DSTREG);							\
	*rd = RS##REG(RS##SRCREG);									\
	SET_NZ(*rd);												\
	CLR_V;														\
	COUNT_CYCLES(1);											\
}
static void move_rr_a (void) { MOVE_RR(A,A); }
static void move_rr_b (void) { MOVE_RR(B,B); }
static void move_rr_ax(void) { MOVE_RR(A,B); }
static void move_rr_bx(void) { MOVE_RR(B,A); }

#define MOVE_RN(F,R)	       		       			    		\
{																\
	WFIELD##F(R##REG(R##DSTREG),R##REG(R##SRCREG));				\
	COUNT_CYCLES(1);											\
}
static void move0_rn_a (void) { MOVE_RN(0,A); }
static void move0_rn_b (void) { MOVE_RN(0,B); }
static void move1_rn_a (void) { MOVE_RN(1,A); }
static void move1_rn_b (void) { MOVE_RN(1,B); }

#define MOVE_R_DN(F,R)	       		       			    		\
{																\
	INT32 *rd = &R##REG(R##DSTREG);								\
	*rd-=FW_INC(F);												\
	WFIELD##F(*rd,R##REG(R##SRCREG));							\
	COUNT_CYCLES(2);											\
}
static void move0_r_dn_a (void) { MOVE_R_DN(0,A); }
static void move0_r_dn_b (void) { MOVE_R_DN(0,B); }
static void move1_r_dn_a (void) { MOVE_R_DN(1,A); }
static void move1_r_dn_b (void) { MOVE_R_DN(1,B); }

#define MOVE_R_NI(F,R)	       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
    WFIELD##F(*rd,R##REG(R##SRCREG));							\
    *rd+=FW_INC(F);												\
	COUNT_CYCLES(1);											\
}
static void move0_r_ni_a (void) { MOVE_R_NI(0,A); }
static void move0_r_ni_b (void) { MOVE_R_NI(0,B); }
static void move1_r_ni_a (void) { MOVE_R_NI(1,A); }
static void move1_r_ni_b (void) { MOVE_R_NI(1,B); }

#define MOVE_NR(F,R)	       		       			    		\
{																\
	INT32 *rd = &R##REG(R##DSTREG);								\
	*rd = RFIELD##F(R##REG(R##SRCREG));							\
	SET_NZ(*rd);												\
	CLR_V;														\
	COUNT_CYCLES(3);											\
}
static void move0_nr_a (void) { MOVE_NR(0,A); }
static void move0_nr_b (void) { MOVE_NR(0,B); }
static void move1_nr_a (void) { MOVE_NR(1,A); }
static void move1_nr_b (void) { MOVE_NR(1,B); }

#define MOVE_DN_R(F,R)	       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 *rs = &R##REG(R##SRCREG);								\
	*rs-=FW_INC(F);												\
	*rd = RFIELD##F(*rs);										\
	SET_NZ(*rd);												\
	CLR_V;														\
	COUNT_CYCLES(4);											\
}
static void move0_dn_r_a (void) { MOVE_DN_R(0,A); }
static void move0_dn_r_b (void) { MOVE_DN_R(0,B); }
static void move1_dn_r_a (void) { MOVE_DN_R(1,A); }
static void move1_dn_r_b (void) { MOVE_DN_R(1,B); }

#define MOVE_NI_R(F,R)	       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 *rs = &R##REG(R##SRCREG);								\
	INT32 data = RFIELD##F(*rs);								\
	*rs+=FW_INC(F);												\
	*rd = data;													\
	SET_NZ(*rd);												\
	CLR_V;														\
	COUNT_CYCLES(3);											\
}
static void move0_ni_r_a (void) { MOVE_NI_R(0,A); }
static void move0_ni_r_b (void) { MOVE_NI_R(0,B); }
static void move1_ni_r_a (void) { MOVE_NI_R(1,A); }
static void move1_ni_r_b (void) { MOVE_NI_R(1,B); }

#define MOVE_NN(F,R)	       		       			    		\
{										  						\
	WFIELD##F(R##REG(R##DSTREG),RFIELD##F(R##REG(R##SRCREG)));	\
	COUNT_CYCLES(3);											\
}
static void move0_nn_a (void) { MOVE_NN(0,A); }
static void move0_nn_b (void) { MOVE_NN(0,B); }
static void move1_nn_a (void) { MOVE_NN(1,A); }
static void move1_nn_b (void) { MOVE_NN(1,B); }

#define MOVE_DN_DN(F,R)	       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 *rs = &R##REG(R##SRCREG);								\
	INT32 data;													\
	*rs-=FW_INC(F);												\
	data = RFIELD##F(*rs);										\
	*rd-=FW_INC(F);												\
	WFIELD##F(*rd,data);										\
	COUNT_CYCLES(4);											\
}
static void move0_dn_dn_a (void) { MOVE_DN_DN(0,A); }
static void move0_dn_dn_b (void) { MOVE_DN_DN(0,B); }
static void move1_dn_dn_a (void) { MOVE_DN_DN(1,A); }
static void move1_dn_dn_b (void) { MOVE_DN_DN(1,B); }

#define MOVE_NI_NI(F,R)	       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 *rs = &R##REG(R##SRCREG);								\
	INT32 data = RFIELD##F(*rs);								\
	*rs+=FW_INC(F);												\
	WFIELD##F(*rd,data);										\
	*rd+=FW_INC(F);												\
	COUNT_CYCLES(4);											\
}
static void move0_ni_ni_a (void) { MOVE_NI_NI(0,A); }
static void move0_ni_ni_b (void) { MOVE_NI_NI(0,B); }
static void move1_ni_ni_a (void) { MOVE_NI_NI(1,A); }
static void move1_ni_ni_b (void) { MOVE_NI_NI(1,B); }

#define MOVE_R_NO(F,R)	       		       			    		\
{								  								\
	INT32 o = PARAM_WORD();										\
	WFIELD##F(R##REG(R##DSTREG)+o,R##REG(R##SRCREG));			\
	COUNT_CYCLES(3);											\
}
static void move0_r_no_a (void) { MOVE_R_NO(0,A); }
static void move0_r_no_b (void) { MOVE_R_NO(0,B); }
static void move1_r_no_a (void) { MOVE_R_NO(1,A); }
static void move1_r_no_b (void) { MOVE_R_NO(1,B); }

#define MOVE_NO_R(F,R)	       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 o = PARAM_WORD();										\
	*rd = RFIELD##F(R##REG(R##SRCREG)+o);						\
	SET_NZ(*rd);												\
	CLR_V;														\
	COUNT_CYCLES(5);											\
}
static void move0_no_r_a (void) { MOVE_NO_R(0,A); }
static void move0_no_r_b (void) { MOVE_NO_R(0,B); }
static void move1_no_r_a (void) { MOVE_NO_R(1,A); }
static void move1_no_r_b (void) { MOVE_NO_R(1,B); }

#define MOVE_NO_NI(F,R)	       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 o = PARAM_WORD();										\
	INT32 data = RFIELD##F(R##REG(R##SRCREG)+o);				\
	WFIELD##F(*rd,data);										\
	*rd+=FW_INC(F);												\
	COUNT_CYCLES(5);											\
}
static void move0_no_ni_a (void) { MOVE_NO_NI(0,A); }
static void move0_no_ni_b (void) { MOVE_NO_NI(0,B); }
static void move1_no_ni_a (void) { MOVE_NO_NI(1,A); }
static void move1_no_ni_b (void) { MOVE_NO_NI(1,B); }

#define MOVE_NO_NO(F,R)	       		       			    		\
{				 												\
	INT32 o1 = PARAM_WORD();									\
	INT32 o2 = PARAM_WORD();									\
	INT32 data = RFIELD##F(R##REG(R##SRCREG)+o1);				\
	WFIELD##F(R##REG(R##DSTREG)+o2,data);						\
	COUNT_CYCLES(5);											\
}
static void move0_no_no_a (void) { MOVE_NO_NO(0,A); }
static void move0_no_no_b (void) { MOVE_NO_NO(0,B); }
static void move1_no_no_a (void) { MOVE_NO_NO(1,A); }
static void move1_no_no_b (void) { MOVE_NO_NO(1,B); }

#define MOVE_RA(F,R)	       		       			    		\
{							  									\
	WFIELD##F(PARAM_LONG(),R##REG(R##DSTREG));					\
	COUNT_CYCLES(3);											\
}
static void move0_ra_a (void) { MOVE_RA(0,A); }
static void move0_ra_b (void) { MOVE_RA(0,B); }
static void move1_ra_a (void) { MOVE_RA(1,A); }
static void move1_ra_b (void) { MOVE_RA(1,B); }

#define MOVE_AR(F,R)	       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	*rd = RFIELD##F(PARAM_LONG());								\
	SET_NZ(*rd);												\
	CLR_V;														\
	COUNT_CYCLES(5);											\
}
static void move0_ar_a (void) { MOVE_AR(0,A); }
static void move0_ar_b (void) { MOVE_AR(0,B); }
static void move1_ar_a (void) { MOVE_AR(1,A); }
static void move1_ar_b (void) { MOVE_AR(1,B); }

#define MOVE_A_NI(F,R)	       		       			    		\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
    WFIELD##F(*rd,RFIELD##F(PARAM_LONG()));						\
    *rd+=FW_INC(F);												\
	COUNT_CYCLES(5);											\
}
static void move0_a_ni_a (void) { MOVE_A_NI(0,A); }
static void move0_a_ni_b (void) { MOVE_A_NI(0,B); }
static void move1_a_ni_a (void) { MOVE_A_NI(1,A); }
static void move1_a_ni_b (void) { MOVE_A_NI(1,B); }

#define MOVE_AA(F)		       		       			    		\
{																\
	UINT32 bitaddrs=PARAM_LONG();								\
	WFIELD##F(PARAM_LONG(),RFIELD##F(bitaddrs));				\
	COUNT_CYCLES(7);											\
}
static void move0_aa (void) { MOVE_AA(0); }
static void move1_aa (void) { MOVE_AA(1); }



/*###################################################################################################
**	PROGRAM CONTROL INSTRUCTIONS
**#################################################################################################*/

#define CALL(R)													\
{																\
	PUSH(PC);													\
	PC = R##REG(R##DSTREG);										\
	CORRECT_ODD_PC("CALL");										\
	change_pc29lew(TOBYTE(PC));										\
	COUNT_CYCLES(3);											\
}
static void call_a (void) { CALL(A); }
static void call_b (void) { CALL(B); }

static void callr(void)
{
	PUSH(PC+0x10);
	PC += (PARAM_WORD_NO_INC()<<4)+0x10;
	COUNT_CYCLES(3);
}

static void calla(void)
{
	PUSH(PC+0x20);
	PC = PARAM_LONG_NO_INC();
	CORRECT_ODD_PC("CALLA");
	change_pc29lew(TOBYTE(PC));
	COUNT_CYCLES(4);
}

#define DSJ(R)													\
{																\
	if (--R##REG(R##DSTREG))									\
	{															\
		PC += (PARAM_WORD_NO_INC()<<4)+0x10;					\
		COUNT_CYCLES(3);										\
	}															\
	else														\
	{															\
		SKIP_WORD;												\
		COUNT_CYCLES(2);										\
	}															\
}
static void dsj_a (void) { DSJ(A); }
static void dsj_b (void) { DSJ(B); }

#define DSJEQ(R)												\
{																\
	if (!NOTZ_FLAG)												\
	{															\
		if (--R##REG(R##DSTREG))								\
		{														\
			PC += (PARAM_WORD_NO_INC()<<4)+0x10;				\
			COUNT_CYCLES(3);									\
		}														\
		else													\
		{														\
			SKIP_WORD;											\
			COUNT_CYCLES(2);									\
		}														\
	}															\
	else														\
	{															\
		SKIP_WORD;												\
		COUNT_CYCLES(2);										\
	}															\
}
static void dsjeq_a (void) { DSJEQ(A); }
static void dsjeq_b (void) { DSJEQ(B); }

#define DSJNE(R)												\
{																\
	if (NOTZ_FLAG)												\
	{															\
		if (--R##REG(R##DSTREG))								\
		{														\
			PC += (PARAM_WORD_NO_INC()<<4)+0x10;				\
			COUNT_CYCLES(3);									\
		}														\
		else													\
		{														\
			SKIP_WORD;											\
			COUNT_CYCLES(2);									\
		}														\
	}															\
	else														\
	{															\
		SKIP_WORD;												\
		COUNT_CYCLES(2);										\
	}															\
}
static void dsjne_a (void) { DSJNE(A); }
static void dsjne_b (void) { DSJNE(B); }

#define DSJS(R)													\
{									   							\
	if (state.op & 0x0400)										\
	{															\
		if (--R##REG(R##DSTREG))								\
		{														\
			PC -= ((PARAM_K)<<4);								\
			COUNT_CYCLES(2);									\
		}														\
		else													\
			COUNT_CYCLES(3);									\
	}															\
	else														\
	{															\
		if (--R##REG(R##DSTREG))								\
		{														\
			PC += ((PARAM_K)<<4);								\
			COUNT_CYCLES(2);									\
		}														\
		else													\
			COUNT_CYCLES(3);									\
	}															\
}
static void dsjs_a (void) { DSJS(A); }
static void dsjs_b (void) { DSJS(B); }

static void emu(void)
{
	/* in RUN state, this instruction is a NOP */
	COUNT_CYCLES(6);
}

#define EXGPC(R)												\
{			  													\
	INT32 *rd = &R##REG(R##DSTREG);								\
	INT32 temppc = *rd;											\
	*rd = PC;													\
	PC = temppc;												\
	CORRECT_ODD_PC("EXGPC");									\
	change_pc29lew(TOBYTE(PC));										\
	COUNT_CYCLES(2);											\
}
static void exgpc_a (void) { EXGPC(A); }
static void exgpc_b (void) { EXGPC(B); }

#define GETPC(R)												\
{																\
	R##REG(R##DSTREG) = PC;										\
	COUNT_CYCLES(1);											\
}
static void getpc_a (void) { GETPC(A); }
static void getpc_b (void) { GETPC(B); }

#define GETST(R)												\
{			  													\
	R##REG(R##DSTREG) = GET_ST();								\
	COUNT_CYCLES(1);											\
}
static void getst_a (void) { GETST(A); }
static void getst_b (void) { GETST(B); }

#define j_xx_8(TAKE)			  								\
{	   															\
	if (ADSTREG)												\
	{															\
		if (TAKE)												\
		{														\
			PC += (PARAM_REL8 << 4);							\
			COUNT_CYCLES(2);									\
		}														\
		else													\
			COUNT_CYCLES(1);									\
	}															\
	else														\
	{															\
		if (TAKE)												\
		{														\
			PC = PARAM_LONG_NO_INC();							\
			CORRECT_ODD_PC("J_XX_8");							\
			change_pc29lew(TOBYTE(PC));								\
			COUNT_CYCLES(3);									\
		}											            \
		else													\
		{														\
			SKIP_LONG;											\
			COUNT_CYCLES(4);									\
		}														\
	}															\
}

#define j_xx_0(TAKE)											\
{																\
	if (ADSTREG)												\
	{															\
		if (TAKE)												\
		{														\
			PC += (PARAM_REL8 << 4);							\
			COUNT_CYCLES(2);									\
		}														\
		else													\
			COUNT_CYCLES(1);									\
	}															\
	else														\
	{															\
		if (TAKE)												\
		{														\
			PC += (PARAM_WORD_NO_INC()<<4)+0x10;				\
			COUNT_CYCLES(3);									\
		}														\
		else													\
		{														\
			SKIP_WORD;											\
			COUNT_CYCLES(2);									\
		}														\
	}															\
}

#define j_xx_x(TAKE)											\
{																\
	if (TAKE)													\
	{															\
		PC += (PARAM_REL8 << 4);								\
		COUNT_CYCLES(2);										\
	}															\
	else														\
		COUNT_CYCLES(1);										\
}

static void j_UC_0(void)
{
	j_xx_0(1);
}
static void j_UC_8(void)
{
	j_xx_8(1);
}
static void j_UC_x(void)
{
	j_xx_x(1);
}
static void j_P_0(void)
{
	j_xx_0(!N_FLAG && NOTZ_FLAG);
}
static void j_P_8(void)
{
	j_xx_8(!N_FLAG && NOTZ_FLAG);
}
static void j_P_x(void)
{
	j_xx_x(!N_FLAG && NOTZ_FLAG);
}
static void j_LS_0(void)
{
	j_xx_0(C_FLAG || !NOTZ_FLAG);
}
static void j_LS_8(void)
{
	j_xx_8(C_FLAG || !NOTZ_FLAG);
}
static void j_LS_x(void)
{
	j_xx_x(C_FLAG || !NOTZ_FLAG);
}
static void j_HI_0(void)
{
	j_xx_0(!C_FLAG && NOTZ_FLAG);
}
static void j_HI_8(void)
{
	j_xx_8(!C_FLAG && NOTZ_FLAG);
}
static void j_HI_x(void)
{
	j_xx_x(!C_FLAG && NOTZ_FLAG);
}
static void j_LT_0(void)
{
	j_xx_0((N_FLAG && !V_FLAG) || (!N_FLAG && V_FLAG));
}
static void j_LT_8(void)
{
	j_xx_8((N_FLAG && !V_FLAG) || (!N_FLAG && V_FLAG));
}
static void j_LT_x(void)
{
	j_xx_x((N_FLAG && !V_FLAG) || (!N_FLAG && V_FLAG));
}
static void j_GE_0(void)
{
	j_xx_0((N_FLAG && V_FLAG) || (!N_FLAG && !V_FLAG));
}
static void j_GE_8(void)
{
	j_xx_8((N_FLAG && V_FLAG) || (!N_FLAG && !V_FLAG));
}
static void j_GE_x(void)
{
	j_xx_x((N_FLAG && V_FLAG) || (!N_FLAG && !V_FLAG));
}
static void j_LE_0(void)
{
	j_xx_0((N_FLAG && !V_FLAG) || (!N_FLAG && V_FLAG) || !NOTZ_FLAG);
}
static void j_LE_8(void)
{
	j_xx_8((N_FLAG && !V_FLAG) || (!N_FLAG && V_FLAG) || !NOTZ_FLAG);
}
static void j_LE_x(void)
{
	j_xx_x((N_FLAG && !V_FLAG) || (!N_FLAG && V_FLAG) || !NOTZ_FLAG);
}
static void j_GT_0(void)
{
	j_xx_0((N_FLAG && V_FLAG && NOTZ_FLAG) || (!N_FLAG && !V_FLAG && NOTZ_FLAG));
}
static void j_GT_8(void)
{
	j_xx_8((N_FLAG && V_FLAG && NOTZ_FLAG) || (!N_FLAG && !V_FLAG && NOTZ_FLAG));
}
static void j_GT_x(void)
{
	j_xx_x((N_FLAG && V_FLAG && NOTZ_FLAG) || (!N_FLAG && !V_FLAG && NOTZ_FLAG));
}
static void j_C_0(void)
{
	j_xx_0(C_FLAG);
}
static void j_C_8(void)
{
	j_xx_8(C_FLAG);
}
static void j_C_x(void)
{
	j_xx_x(C_FLAG);
}
static void j_NC_0(void)
{
	j_xx_0(!C_FLAG);
}
static void j_NC_8(void)
{
	j_xx_8(!C_FLAG);
}
static void j_NC_x(void)
{
	j_xx_x(!C_FLAG);
}
static void j_EQ_0(void)
{
	j_xx_0(!NOTZ_FLAG);
}
static void j_EQ_8(void)
{
	j_xx_8(!NOTZ_FLAG);
}
static void j_EQ_x(void)
{
	j_xx_x(!NOTZ_FLAG);
}
static void j_NE_0(void)
{
	j_xx_0(NOTZ_FLAG);
}
static void j_NE_8(void)
{
	j_xx_8(NOTZ_FLAG);
}
static void j_NE_x(void)
{
	j_xx_x(NOTZ_FLAG);
}
static void j_V_0(void)
{
	j_xx_0(V_FLAG);
}
static void j_V_8(void)
{
	j_xx_8(V_FLAG);
}
static void j_V_x(void)
{
	j_xx_x(V_FLAG);
}
static void j_NV_0(void)
{
	j_xx_0(!V_FLAG);
}
static void j_NV_8(void)
{
	j_xx_8(!V_FLAG);
}
static void j_NV_x(void)
{
	j_xx_x(!V_FLAG);
}
static void j_N_0(void)
{
	j_xx_0(N_FLAG);
}
static void j_N_8(void)
{
	j_xx_8(N_FLAG);
}
static void j_N_x(void)
{
	j_xx_x(N_FLAG);
}
static void j_NN_0(void)
{
	j_xx_0(!N_FLAG);
}
static void j_NN_8(void)
{
	j_xx_8(!N_FLAG);
}
static void j_NN_x(void)
{
	j_xx_x(!N_FLAG);
}

#define JUMP(R)													\
{																\
	PC = R##REG(R##DSTREG);										\
	CORRECT_ODD_PC("JUMP");										\
	change_pc29lew(TOBYTE(PC));										\
	COUNT_CYCLES(2);											\
}
static void jump_a (void) { JUMP(A); }
static void jump_b (void) { JUMP(B); }

static void popst(void)
{
	SET_ST(POP());
	COUNT_CYCLES(8);
}

static void pushst(void)
{
	PUSH(GET_ST());
	COUNT_CYCLES(2);
}

#define PUTST(R)												\
{																\
	SET_ST(R##REG(R##DSTREG));									\
	COUNT_CYCLES(3);											\
}
static void putst_a (void) { PUTST(A); }
static void putst_b (void) { PUTST(B); }

static void reti(void)
{
	INT32 st = POP();
	PC = POP();
	CORRECT_ODD_PC("RETI");
	change_pc29lew(TOBYTE(PC));
	SET_ST(st);
	COUNT_CYCLES(11);
}

static void rets(void)
{
	UINT32 offs;
	PC = POP();
	CORRECT_ODD_PC("RETS");
	change_pc29lew(TOBYTE(PC));
	offs = PARAM_N;
	if (offs)
	{
		SP+=(offs<<4);
	}
	COUNT_CYCLES(7);
}

#define REV(R)													\
{																\
    R##REG(R##DSTREG) = 0x0008;									\
	COUNT_CYCLES(1);											\
}
static void rev_a (void) { REV(A); }
static void rev_b (void) { REV(B); }

static void trap(void)
{
	UINT32 t = PARAM_N;
	if (t)
	{
		PUSH(PC);
		PUSH(GET_ST());
	}
	RESET_ST();
	PC = RLONG(0xffffffe0-(t<<5));
	CORRECT_ODD_PC("TRAP");
	change_pc29lew(TOBYTE(PC));
	COUNT_CYCLES(16);
}



/*###################################################################################################
**	34020 INSTRUCTIONS
**#################################################################################################*/

/************************************

New 34020 ops:

	0000 1100 000R dddd = ADDXYI IL,Rd
	iiii iiii iiii iiii
	iiii iiii iiii iiii

	0000 0000 1111 00SD = BLMOVE S,D

	0000 0110 0000 0000 = CEXEC S,c,ID,L
	cccc cccc S000 0000
	iiic cccc cccc cccc

	1101 1000 0ccc cccS = CEXEC S,c,ID
	iiic cccc cccc cccc

	0000 1000 1111 0010 = CLIP

	0000 0110 011R dddd = CMOVCG Rd1,Rd2,S,c,ID
	cccc cccc S00R dddd
	iiic cccc cccc cccc

	0000 0110 101R dddd = CMOVCM *Rd+,n,S,c,ID
	cccc cccc S00n nnnn
	iiic cccc cccc cccc

	0000 0110 110R dddd = CMOVCM -*Rd,n,S,c,ID
	cccc cccc S00n nnnn
	iiic cccc cccc cccc

	0000 0110 0110 0000 = CMOVCS c,ID
	cccc cccc 0000 0001
	iiic cccc cccc cccc

	0000 0110 001R ssss = CMOVGC Rs,c,ID
	cccc cccc 0000 0000
	iiic cccc cccc cccc

	0000 0110 010R ssss = CMOVGC Rs1,Rs2,S,c,ID
	cccc cccc S00R ssss
	iiic cccc cccc cccc

	0000 0110 100n nnnn = CMOVMC *Rs+,n,S,c,ID
	cccc cccc S00R ssss
	iiic cccc cccc cccc

	0000 1000 001n nnnn = CMOVMC -*Rs,n,S,c,ID
	cccc cccc S00R ssss
	iiic cccc cccc cccc

	0000 0110 111R dddd = CMOVMC *Rs+,Rd,S,c,ID
	cccc cccc S00R ssss
	iiic cccc cccc cccc

	0011 01kk kkkR dddd = CMPK k,Rd

	0000 1010 100R dddd = CVDXYL Rd

	0000 1010 011R dddd = CVMXYL Rd

	1110 101s sssR dddd = CVSXYL Rs,Rd

	0000 0010 101R dddd = EXGPS Rd

	1101 1110 Z001 1010 = FLINE Z

	0000 1010 1011 1011 = FPIXEQ

	0000 1010 1101 1011 = FPIXNE

	0000 0010 110R dddd = GETPS Rd

	0000 0000 0100 0000 = IDLE

	0000 1100 0101 0111 = LINIT

	0000 0000 1000 0000 = MWAIT

	0000 1010 0011 0111 = PFILL XY

	0000 1110 0001 0111 = PIXBLT L,M,L

	0000 1000 0110 0000 = RETM

	0111 101s sssR dddd = RMO Rs,Rd

	0000 0010 100R dddd = RPIX Rd

	0000 0010 0111 0011 = SETCDP

	0000 0010 1111 1011 = SETCMP

	0000 0010 0101 0001 = SETCSP

	0111 111s sssR dddd = SWAPF *Rs,Rd,0

	0000 1110 1111 1010 = TFILL XY

	0000 1000 0000 1111 = TRAPL

	0000 1000 0101 0111 = VBLT B,L

	0000 1010 0101 0111 = VFILL L

	0000 1010 0000 0000 = VLCOL

************************************/


#define ADD_XYI(R)								\
{												\
	UINT32 a = PARAM_LONG();					\
	XY *b = &R##REG_XY(R##DSTREG);				\
	XY res;										\
	res.x = b->x + (INT16)(a & 0xffff);			\
	   N_FLAG = !res.x;							\
	   V_FLAG = res.x & 0x8000;					\
	res.y = b->y + ((INT32)a >> 16);			\
	NOTZ_FLAG = res.y;							\
	   C_FLAG = res.y & 0x8000;					\
  	*b = res;									\
  	COUNT_CYCLES(1);							\
}
static void addxyi_a(void)
{
	if (!state.is_34020) { unimpl(); return; }
	ADD_XYI(A);
}
static void addxyi_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	ADD_XYI(B);
}

static void blmove(void)
{
	offs_t src = BREG(BINDEX(0));
	offs_t dst = BREG(BINDEX(2));
	offs_t bits = BREG(BINDEX(7));

	if (!state.is_34020) { unimpl(); return; }

	/* src and dst are aligned */
	if (!(src & 0x0f) && !(dst & 0x0f))
	{
		while (bits >= 16 && tms34010_ICount > 0)
		{
			TMS34010_WRMEM_WORD(TOBYTE(dst), TMS34010_RDMEM_WORD(TOBYTE(src)));
			src += 0x10;
			dst += 0x10;
			bits -= 0x10;
			tms34010_ICount -= 2;
		}
		if (bits != 0 && tms34010_ICount > 0)
		{
			(*wfield_functions[bits])(dst, (*rfield_functions_z[bits])(src));
			dst += bits;
			src += bits;
			bits = 0;
			tms34010_ICount -= 2;
		}
	}

	/* src is aligned, dst is not */
	else if (!(src & 0x0f))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "020:BLMOVE with aligned src and unaligned dst\n");
	}

	/* dst is aligned, src is not */
	else if (!(dst & 0x0f))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "020:BLMOVE with unaligned src and aligned dst\n");
	}

	/* neither are aligned */
	else
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "020:BLMOVE with completely unaligned src and dst\n");
	}

	/* update the final results */
	BREG(BINDEX(0)) = src;
	BREG(BINDEX(2)) = dst;
	BREG(BINDEX(7)) = bits;

	/* if we're not done yet, back up the PC */
	if (bits != 0)
		PC -= 0x10;
}

static void cexec_l(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cexec_l\n");*/
}

static void cexec_s(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cexec_s\n");*/
}

static void clip(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:clip\n");*/
}

static void cmovcg_a(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cmovcg_a\n");*/
}

static void cmovcg_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cmovcg_b\n");*/
}

static void cmovcm_f(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cmovcm_f\n");*/
}

static void cmovcm_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cmovcm_b\n");*/
}

static void cmovgc_a(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cmovgc_a\n");*/
}

static void cmovgc_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cmovgc_b\n");*/
}

static void cmovgc_a_s(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cmovgc_a_s\n");*/
}

static void cmovgc_b_s(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cmovgc_b_s\n");*/
}

static void cmovmc_f(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cmovmc_f\n");*/
}

static void cmovmc_f_va(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cmovmc_f_va\n");*/
}

static void cmovmc_f_vb(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cmovmc_f_vb\n");*/
}

static void cmovmc_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cmovmc_b\n");*/
}

#define CMPK(R)				       		       			    \
{															\
	INT32 r;												\
	INT32 *rd = &R##REG(R##DSTREG);							\
	INT32 t = PARAM_K; if (!t) t = 32;						\
	r = *rd - t;											\
	SET_NZCV_SUB(*rd,t,r);									\
	COUNT_CYCLES(1);										\
}
static void cmp_k_a(void)
{
	if (!state.is_34020) { unimpl(); return; }
	CMPK(A);
}
static void cmp_k_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	CMPK(B);
}

static void cvdxyl_a(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cvdxyl_a\n");*/
}

static void cvdxyl_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cvdxyl_b\n");*/
}

static void cvmxyl_a(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cvmxyl_a\n");*/
}

static void cvmxyl_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cvmxyl_b\n");*/
}

static void cvsxyl_a(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cvsxyl_a\n");*/
}

static void cvsxyl_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:cvsxyl_b\n");*/
}

static void exgps_a(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:exgps_a\n");*/
}

static void exgps_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:exgps_b\n");*/
}

static void fline(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:fline\n");*/
}

static void fpixeq(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:fpixeq\n");*/
}

static void fpixne(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:fpixne\n");*/
}

static void getps_a(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:getps_a\n");*/
}

static void getps_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:getps_b\n");*/
}

static void idle(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:idle\n");*/
}

static void linit(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:linit\n");*/
}

static void mwait(void)
{
	if (!state.is_34020) { unimpl(); return; }
}

static void pfill_xy(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:pfill_xy\n");*/
}

static void pixblt_l_m_l(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:pixblt_l_m_l\n");*/
}

static void retm(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:retm\n");*/
}

#define RMO(R)			       		       			    		\
{																\
	UINT32 res = 0;												\
	UINT32 rs  = R##REG(R##SRCREG);								\
	 INT32 *rd = &R##REG(R##DSTREG);							\
	SET_Z(rs);													\
	if (rs)														\
	{															\
		while (!(rs & 0x00000001))								\
		{														\
			res++;												\
			rs >>= 1;											\
		}														\
	}															\
	*rd = res;													\
	COUNT_CYCLES(1);											\
}

static void rmo_a(void) { RMO(A); }
static void rmo_b(void) { RMO(B); }

#define RPIX(R)									\
{												\
	UINT32 v = R##REG(R##DSTREG);				\
	switch (state.pixelshift)					\
	{											\
		case 1:									\
			v = (v & 1) ? 0xffffffff : 0x00000000;\
		  	COUNT_CYCLES(8);					\
		  	break;								\
		case 2:									\
			v &= 3;								\
			v |= v << 2;						\
			v |= v << 4;						\
			v |= v << 8;						\
			v |= v << 16;						\
			COUNT_CYCLES(7);					\
			break;								\
		case 4:									\
			v &= 0x0f;							\
			v |= v << 4;						\
			v |= v << 8;						\
			v |= v << 16;						\
			COUNT_CYCLES(6);					\
			break;								\
		case 8:									\
			v &= 0xff;							\
			v |= v << 8;						\
			v |= v << 16;						\
			COUNT_CYCLES(5);					\
			break;								\
		case 16:								\
			v &= 0xffff;						\
			v |= v << 16;						\
			COUNT_CYCLES(4);					\
			break;								\
		case 32:								\
			COUNT_CYCLES(2);					\
			break;								\
	}											\
	R##REG(R##DSTREG) = v;						\
}

static void rpix_a(void)
{
	if (!state.is_34020) { unimpl(); return; }
	RPIX(A);
}

static void rpix_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	RPIX(B);
}

static void setcdp(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:setcdp\n");*/
}

static void setcmp(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:setcmp\n");*/
}

static void setcsp(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:setcsp\n");*/
}

static void swapf_a(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:swapf_a\n");*/
}

static void swapf_b(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:swapf_b\n");*/
}

static void tfill_xy(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:tfill_xy\n");*/
}

static void trapl(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:trapl\n");*/
}

static void vblt_b_l(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:vblt_b_l\n");*/
}

static void vfill_l(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:vfill_l\n");*/
}

static void vlcol(void)
{
	if (!state.is_34020) { unimpl(); return; }
	/*logerror("020:vlcol\n");*/
}
