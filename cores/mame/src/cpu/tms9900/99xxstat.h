/************************************************************************

	ST register functions

************************************************************************/

/*
	remember that the OP ST bit is maintained in lastparity
*/

/*
	setstat sets the ST_OP bit according to lastparity

	It must be called before reading the ST register.
*/

static void setstat(void)
{
	int i;
	UINT8 a;

	I.STATUS &= ~ ST_OP;

	/* We set the parity bit. */
	a = lastparity;

	for (i=0; i<8; i++)     /* 8 bits to test */
	{
		if (a & 1)  /* If current bit is set */
			I.STATUS ^= ST_OP;  /* we toggle the ST_OP bit */

		a >>= 1;    /* Next bit. */
	}
}

/*
	getstat sets emulator's lastparity variable according to 9900's STATUS bits.
	It must be called on interrupt return, or when, for some reason,
	the emulated program sets the STATUS register directly.
*/
static void getstat(void)
{
#if TMS99XX_MODEL <= TMS9985_ID
	I.STATUS &= ST_MASK;  /* unused bits are forced to 0 */
#endif

	if (I.STATUS & ST_OP)
		lastparity = 1;
	else
		lastparity = 0;
}

/*
	A few words about the following functions.

	A big portability issue is the behavior of the ">>" instruction with the sign bit, which has
	not been normalised.  Every compiler does whatever it thinks smartest.
	My code assumed that when shifting right signed numbers, the operand is left-filled with a
	copy of sign bit, and that when shifting unsigned variables, it is left-filled with 0s.
	This is probably the most logical behaviour, and it is the behavior of CW PRO3 - most time
	(the exception is that ">>=" instructions always copy the sign bit (!)).  But some compilers
	are bound to disagree.

	So, I had to create special functions with predefined tables included, so that this code work
	on every compiler.  BUT this is a real slow-down.
	So, you might have to include a few lines in assembly to make this work better.
	Sorry about this, this problem is really unpleasant and absurd, but it is not my fault.
*/


static const UINT16 right_shift_mask_table[17] =
{
	0xFFFF,
	0x7FFF,
	0x3FFF,
	0x1FFF,
	0x0FFF,
	0x07FF,
	0x03FF,
	0x01FF,
	0x00FF,
	0x007F,
	0x003F,
	0x001F,
	0x000F,
	0x0007,
	0x0003,
	0x0001,
	0x0000
};

static const UINT16 inverted_right_shift_mask_table[17] =
{
	0x0000,
	0x8000,
	0xC000,
	0xE000,
	0xF000,
	0xF800,
	0xFC00,
	0xFE00,
	0xFF00,
	0xFF80,
	0xFFC0,
	0xFFE0,
	0xFFF0,
	0xFFF8,
	0xFFFC,
	0xFFFE,
	0xFFFF
};

static INLINE UINT16 logical_right_shift(UINT16 val, int c)
{
	return((val>>c) & right_shift_mask_table[c]);
}

static INLINE INT16 arithmetic_right_shift(INT16 val, int c)
{
	if (val < 0)
		return((val>>c) | inverted_right_shift_mask_table[c]);
	else
		return((val>>c) & right_shift_mask_table[c]);
}





/*
	Set lae
*/
static INLINE void setst_lae(INT16 val)
{
	I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ);

	if (val > 0)
		I.STATUS |= (ST_LGT | ST_AGT);
	else if (val < 0)
		I.STATUS |= ST_LGT;
	else
		I.STATUS |= ST_EQ;
}


/*
	Set laep (BYTE)
*/
static INLINE void setst_byte_laep(INT8 val)
{
	I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ);

	if (val > 0)
		I.STATUS |= (ST_LGT | ST_AGT);
	else if (val < 0)
		I.STATUS |= ST_LGT;
	else
		I.STATUS |= ST_EQ;

	lastparity = val;
}

/*
	For COC, CZC, and TB
*/
static INLINE void setst_e(UINT16 val, UINT16 to)
{
	if (val == to)
		I.STATUS |= ST_EQ;
	else
		I.STATUS &= ~ ST_EQ;
}

/*
	For CI, C, CB
*/
static INLINE void setst_c_lae(UINT16 to, UINT16 val)
{
	I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ);

	if (val == to)
		I.STATUS |= ST_EQ;
	else
	{
		if ( ((INT16) val) > ((INT16) to) )
			I.STATUS |= ST_AGT;
		if ( ((UINT16) val) > ((UINT16) to) )
		I.STATUS |= ST_LGT;
	}
}

#define wadd(addr,expr) { int lval = setst_add_laeco(readword(addr), (expr)); writeword((addr),lval); }
#define wsub(addr,expr) { int lval = setst_sub_laeco(readword(addr), (expr)); writeword((addr),lval); }

#if defined(__POWERPC__) && !defined(__GNUC__) && !defined(_XBOX)

/* setst_add_32_laeco :*/
/* - computes a+b*/
/* - sets L, A, E, Carry and Overflow in st*/
/**/
/* a -> r3, b -> r4, st -> r5*/
/* preserve r6-r12*/

static INT32 asm setst_add_32_laeco(register INT32 a, register INT32 b, register INT16 st)
{
#if (TMS99XX_MODEL == TMS9940_ID)
  mr r6,a           /* save operand*/
#endif
  addco. r3, b, a   /* add, set CR0, and set CA and OV*/
  clrlwi st, st, 21 /* clear L, A, E, C, O flags (-> keep bits 21-31)*/
  mcrxr cr1         /* move XER (-> CA and CO bits) to CR1*/

  beq- null_result  /* if EQ is set, jump to null_result.  No jump is more likely than a jump.*/
  bgt+ positive_result  /* if GT is set, jump to positive_result.  A jump is more likely than no jump.*/
  ori st, st, ST_LGT    /* if result < 0, set ST_LGT*/
  b next    /* now set carry & overflow as needed*/

null_result:
  ori st, st, ST_EQ /* if result == 0, set ST_EQ*/
  b next

positive_result:
  ori st, st, ST_LGT | ST_AGT   /* if result > 0, set ST_LGT and ST_AGT*/

next:
#if (TMS99XX_MODEL == TMS9940_ID)
  andi. st, st, (~ ST_DC) & 0xFFFF

  or r7,r6,b
  and r6,r6,b
  andc r7,r7,r3
  or r6,r7,r6
  andis. r6,r6,0x0800
  beq+ nodecimalcarry
  ori st, st, ST_DC
nodecimalcarry:
#endif

  bf+ cr1*4+2, nocarry  /* if CA is not set, jump to nocarry.  A jump is more likely than no jump.*/
  ori st, st, ST_C  /* else set ST_C*/
nocarry:
  bflr+ cr1*4+1     /* if OV is not set, return.  A jump is more likely than no jump.*/
  ori st, st, ST_OV /* else set ST_OV*/
  blr   /* return*/
}

/* setst_sub_32_laeco :*/
/* - computes a-b*/
/* - sets L, A, E, Carry and Overflow in ST*/
/**/
/* a -> r3, b -> r4, st -> r5*/
/* preserve r6-r12*/

static INT32 asm setst_sub_32_laeco(register INT32 a, register INT32 b, register INT16 st)
{
#if (TMS99XX_MODEL == TMS9940_ID)
  mr r6,a           /* save operand*/
#endif
  subco. r3, a, b   /* sub, set CR0, and set CA and OV*/
  clrlwi st, st, 21 /* clear L, A, E, C, O flags (-> keep bits 21-31)*/
  mcrxr cr1         /* move XER (-> CA and CO bits) to CR1*/

  beq- null_result  /* if EQ is set, jump to null_result.  No jump is more likely than a jump.*/
  bgt+ positive_result  /* if GT is set, jump to positive_result.  A jump is more likely than no jump.*/
  ori st, st, ST_LGT    /* if result < 0, set ST_LGT*/
  b next    /* now set carry & overflow as needed*/

null_result:
  ori st, st, ST_EQ /* if result == 0, set ST_EQ*/
  b next

positive_result:
  ori st, st, ST_LGT | ST_AGT   /* if result > 0, set ST_LGT and ST_AGT*/

next:
#if (TMS99XX_MODEL == TMS9940_ID)
  andi. st, st, (~ ST_DC) & 0xFFFF

  orc r7,r6,b
  andc r6,r6,b
  andc r7,r7,r3
  or r6,r7,r6
  andis. r6,r6,0x0800
  beq- nodecimalcarry
  ori st, st, ST_DC
nodecimalcarry:
#endif

  bf- cr1*4+2, nocarry  /* if CA is not set, jump to nocarry.  No jump is more likely than a jump.*/
  ori st, st, ST_C  /* else set ST_C*/
nocarry:
  bflr+ cr1*4+1     /* if OV is not set, return.  A jump is more likely than no jump.*/
  ori st, st, ST_OV /* else set ST_OV*/
  blr   /* return*/
}

/**/
/* Set laeco for add*/
/**/
static INT16 asm setst_add_laeco(register INT16 a, register INT16 b)
{ /* a -> r3, b -> r4*/
/*  lwz r6, I(RTOC)   */ /* load pointer to I*/
  _asm_get_global(r6,I)

  slwi a, a, 16     /* shift a*/
  slwi b, b, 16     /* shift b*/

  lhz r5, 4(r6)     /* load ST*/

  mflr r12  /* save LR (we KNOW setst_add_32_laeco will not alter this register)*/

  bl setst_add_32_laeco /* perform addition and set flags*/

  mtlr r12  /* restore LR*/
  srwi r3, r3, 16   /* shift back result*/
  sth r5, 4(r6)     /* save new ST*/
  blr       /* and return*/
}

/**/
/*  Set laeco for subtract*/
/**/
static INT16 asm setst_sub_laeco(register INT16 a, register INT16 b)
{
/*  lwz r6, I(RTOC)*/
  _asm_get_global(r6,I)

  slwi a, a, 16
  slwi b, b, 16

  lhz r5, 4(r6)

  mflr r12

  bl setst_sub_32_laeco /* perform substraction and set flags*/

  mtlr r12
  srwi r3, r3, 16
  sth r5, 4(r6)
  blr
}

/**/
/* Set laecop for add (BYTE)*/
/**/
static INT8 asm setst_addbyte_laecop(register INT8 a, register INT8 b)
{ /* a -> r3, b -> r4*/
/*  lwz r6, I(RTOC)*/
  _asm_get_global(r6,I)

  slwi a, a, 24     /* shift a*/
  slwi b, b, 24     /* shift b*/

  lhz r5, 4(r6)     /* load ST*/

  mflr r12  /* save LR (we KNOW setst_add_32_laeco will not alter this register)*/

  bl setst_add_32_laeco /* perform addition and set flags*/

  srwi r3, r3, 24   /* shift back result*/
  mtlr r12  /* restore LR*/
  sth r5, 4(r6)     /* save new ST*/
/*  stb r3, lastparity(RTOC)  */ /* copy result to lastparity*/
  _asm_set_global_b(r3,lastparity)  /* copy result to lastparity*/
  blr       /* and return*/
}

/**/
/* Set laecop for subtract (BYTE)*/
/**/
static INT8 asm setst_subbyte_laecop(register INT8 a, register INT8 b)
{ /* a -> r3, b -> r4*/
/*  lwz r6, I(RTOC)*/
  _asm_get_global(r6,I)

  slwi a, a, 24
  slwi b, b, 24

  lhz r5, 4(r6)

  mflr r12

  bl setst_sub_32_laeco /* perform substraction and set flags*/

  srwi r3, r3, 24
  mtlr r12
  sth r5, 4(r6)
/*  stb r3, lastparity(RTOC)*/
  _asm_set_global_b(r3,lastparity)  /* copy result to lastparity*/
  blr
}

#else

/* Could do with some equivalent functions for non power PC's */

/*
	Set laeco for add
*/
static INLINE INT16 setst_add_laeco(int a, int b)
{
	UINT32 res;
	INT16 res2;

	I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ | ST_C | ST_OV);

	res = (a & 0xffff) + (b & 0xffff);

	if (res & 0x10000)
		I.STATUS |= ST_C;

	if ((res ^ b) & (res ^ a) & 0x8000)
		I.STATUS |= ST_OV;

#if (TMS99XX_MODEL == TMS9940_ID)
	if (((a & b) | ((a | b) & ~ res)) & 0x0800)
		I.STATUS |= ST_DC;
#endif

	res2 = (INT16) res;

	if (res2 > 0)
		I.STATUS |= ST_LGT | ST_AGT;
	else if (res2 < 0)
		I.STATUS |= ST_LGT;
	else
		I.STATUS |= ST_EQ;

	return res2;
}


/*
	Set laeco for subtract
*/
static INLINE INT16 setst_sub_laeco(int a, int b)
{
	UINT32 res;
	INT16 res2;

	I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ | ST_C | ST_OV);

	res = (a & 0xffff) - (b & 0xffff);

	if (! (res & 0x10000))
		I.STATUS |= ST_C;

	if ((a ^ b) & (a ^ res) & 0x8000)
		I.STATUS |= ST_OV;

#if (TMS99XX_MODEL == TMS9940_ID)
	if (((a & ~ b) | ((a | ~ b) & ~ res)) & 0x0800)
		I.STATUS |= ST_DC;
#endif

	res2 = (INT16) res;

	if (res2 > 0)
		I.STATUS |= ST_LGT | ST_AGT;
	else if (res2 < 0)
		I.STATUS |= ST_LGT;
	else
		I.STATUS |= ST_EQ;

	return res2;
}


/*
	Set laecop for add (BYTE)
*/
static INLINE INT8 setst_addbyte_laecop(int a, int b)
{
	unsigned int res;
	INT8 res2;

	I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ | ST_C | ST_OV | ST_OP);

	res = (a & 0xff) + (b & 0xff);

	if (res & 0x100)
		I.STATUS |= ST_C;

	if ((res ^ b) & (res ^ a) & 0x80)
		I.STATUS |= ST_OV;

#if (TMS99XX_MODEL == TMS9940_ID)
	if (((a & b) | ((a | b) & ~ res)) & 0x08)
		I.STATUS |= ST_DC;
#endif

	res2 = (INT8) res;

	if (res2 > 0)
		I.STATUS |= ST_LGT | ST_AGT;
	else if (res2 < 0)
		I.STATUS |= ST_LGT;
	else
		I.STATUS |= ST_EQ;

	lastparity = res2;

	return res2;
}


/*
	Set laecop for subtract (BYTE)
*/
static INLINE INT8 setst_subbyte_laecop(int a, int b)
{
	unsigned int res;
	INT8 res2;

	I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ | ST_C | ST_OV | ST_OP);

	res = (a & 0xff) - (b & 0xff);

	if (! (res & 0x100))
		I.STATUS |= ST_C;

	if ((a ^ b) & (a ^ res) & 0x80)
		I.STATUS |= ST_OV;

#if (TMS99XX_MODEL == TMS9940_ID)
	if (((a & ~ b) | ((a | ~ b) & ~ res)) & 0x08)
		I.STATUS |= ST_DC;
#endif

	res2 = (INT8) res;

	if (res2 > 0)
		I.STATUS |= ST_LGT | ST_AGT;
	else if (res2 < 0)
		I.STATUS |= ST_LGT;
	else
		I.STATUS |= ST_EQ;

	lastparity = res2;

	return res2;
}

#endif



/*
	For NEG
*/
static INLINE void setst_laeo(INT16 val)
{
	I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ | ST_OV);

	if (val > 0)
		I.STATUS |= ST_LGT | ST_AGT;
	else if (val < 0)
	{
	I.STATUS |= ST_LGT;
	if (((UINT16) val) == 0x8000)
		I.STATUS |= ST_OV;
	}
	else
		I.STATUS |= ST_EQ;
}



/*
	Meat of SRA
*/
static INLINE UINT16 setst_sra_laec(INT16 a, UINT16 c)
{
	I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ | ST_C);

	if (c != 0)
	{
		a = arithmetic_right_shift(a, c-1);
		if (a & 1)  /* The carry bit equals the last bit that is shifted out*/
			I.STATUS |= ST_C;
		a = arithmetic_right_shift(a, 1);
	}

	if (a > 0)
		I.STATUS |= ST_LGT | ST_AGT;
	else if (a < 0)
		I.STATUS |= ST_LGT;
	else
		I.STATUS |= ST_EQ;

	return a;
}


/*
	Meat of SRL.  Same algorithm as SRA, except that we fills in with 0s.
*/
static INLINE UINT16 setst_srl_laec(UINT16 a,UINT16 c)
{
	I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ | ST_C);

	if (c != 0)
	{
		a = logical_right_shift(a, c-1);
		if (a & 1)
			I.STATUS |= ST_C;
		a = logical_right_shift(a, 1);
	}

	if (((INT16) a) > 0)
		I.STATUS |= ST_LGT | ST_AGT;
	else if (((INT16) a) < 0)
		I.STATUS |= ST_LGT;
	else
		I.STATUS |= ST_EQ;

	return a;
}


/**/
/* Meat of SRC*/
/**/
static INLINE UINT16 setst_src_laec(UINT16 a,UINT16 c)
{
	I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ | ST_C);

	if (c != 0)
	{
		a = logical_right_shift(a, c) | (a << (16-c));
		if (a & 0x8000) /* The carry bit equals the last bit that is shifted out*/
			I.STATUS |= ST_C;
	}

	if (((INT16) a) > 0)
		I.STATUS |= ST_LGT | ST_AGT;
	else if (((INT16) a) < 0)
		I.STATUS |= ST_LGT;
	else
		I.STATUS |= ST_EQ;

	return a;
}


/**/
/* Meat of SLA*/
/**/
static INLINE UINT16 setst_sla_laeco(UINT16 a, UINT16 c)
{
	I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ | ST_C | ST_OV);

	if (c != 0)
	{
		{
			register UINT16 mask;
			register UINT16 ousted_bits;

			mask = 0xFFFF << (16-c-1);
			ousted_bits = a & mask;

			if (ousted_bits)        /* If ousted_bits is neither all 0s*/
				if (ousted_bits ^ mask)   /* nor all 1s,*/
					I.STATUS |= ST_OV;  /* we set overflow*/
		}

		a <<= c-1;
		if (a & 0x8000) /* The carry bit equals the last bit that is shifted out*/
			I.STATUS |= ST_C;

		a <<= 1;
	}

	if (((INT16) a) > 0)
		I.STATUS |= ST_LGT | ST_AGT;
	else if (((INT16) a) < 0)
		I.STATUS |= ST_LGT;
	else
		I.STATUS |= ST_EQ;

	return a;
}


/***********************************************************************/
