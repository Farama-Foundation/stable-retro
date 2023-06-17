
/*

HNZC

? = undefined
* = affected
- = unaffected
0 = cleared
1 = set
# = ccr directly affected by instruction
@ = special - carry set if bit 7 is set

*/

static void illegal( void )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "M6805: illegal opcode\n");
}

#ifdef macintosh
#pragma mark ____0x____
#endif

/* $00/$02/$04/$06/$08/$0A/$0C/$0E BRSET direct,relative ---- */
static INLINE void brset (UINT8 bit)
{
	UINT8 t,r;
	DIRBYTE(r);
	IMMBYTE(t);

	CLC;

	if (r&bit) {
		SEC;
		PC+=SIGNED(t);

		if (t==0xfd)
		{
			/* speed up busy loops */
			if(m6805_ICount > 0)
				m6805_ICount = 0;
		}
	}
}

/* $01/$03/$05/$07/$09/$0B/$0D/$0F BRCLR direct,relative ---- */
static INLINE void brclr (UINT8 bit)
{
	UINT8 t,r;
	DIRBYTE(r);
	IMMBYTE(t);

	SEC;

	if (!(r&bit)) {
		CLC;
		PC+=SIGNED(t);

		if (t==0xfd)
		{
			/* speed up busy loops */
			if(m6805_ICount > 0)
				m6805_ICount = 0;
	    }
	}
}


#ifdef macintosh
#pragma mark ____1x____
#endif

/* $10/$12/$14/$16/$18/$1A/$1C/$1E BSET direct ---- */
static INLINE void bset (UINT8 bit)
{
	UINT8 t,r;
	DIRBYTE(t); r=t|bit;
	WM(EAD,r);
}

/* $11/$13/$15/$17/$19/$1B/$1D/$1F BCLR direct ---- */
static INLINE void bclr (UINT8 bit)
{
	UINT8 t,r;
	DIRBYTE(t); r=t&(~bit);
	WM(EAD,r);
}


#ifdef macintosh
#pragma mark ____2x____
#endif

/* $20 BRA relative ---- */
static INLINE void bra( void )
{
	UINT8 t;
	IMMBYTE(t);PC+=SIGNED(t);
	if (t==0xfe)
	{
		/* speed up busy loops */
		if(m6805_ICount > 0)
			m6805_ICount = 0;
    }
}

/* $21 BRN relative ---- */
static INLINE void brn( void )
{
	UINT8 t;
	IMMBYTE(t);
}

/* $22 BHI relative ---- */
static INLINE void bhi( void )
{
	BRANCH( !(CC&(CFLAG|ZFLAG)) );
}

/* $23 BLS relative ---- */
static INLINE void bls( void )
{
	BRANCH( CC&(CFLAG|ZFLAG) );
}

/* $24 BCC relative ---- */
static INLINE void bcc( void )
{
	BRANCH( !(CC&CFLAG) );
}

/* $25 BCS relative ---- */
static INLINE void bcs( void )
{
	BRANCH( CC&CFLAG );
}

/* $26 BNE relative ---- */
static INLINE void bne( void )
{
	BRANCH( !(CC&ZFLAG) );
}

/* $27 BEQ relative ---- */
static INLINE void beq( void )
{
	BRANCH( CC&ZFLAG );
}

/* $28 BHCC relative ---- */
static INLINE void bhcc( void )
{
	BRANCH( !(CC&HFLAG) );
}

/* $29 BHCS relative ---- */
static INLINE void bhcs( void )
{
	BRANCH( CC&HFLAG );
}

/* $2a BPL relative ---- */
static INLINE void bpl( void )
{
	BRANCH( !(CC&NFLAG) );
}

/* $2b BMI relative ---- */
static INLINE void bmi( void )
{
	BRANCH( CC&NFLAG );
}

/* $2c BMC relative ---- */
static INLINE void bmc( void )
{
	BRANCH( !(CC&IFLAG) );
}

/* $2d BMS relative ---- */
static INLINE void bms( void )
{
	BRANCH( CC&IFLAG );
}

/* $2e BIL relative ---- */
static INLINE void bil( void )
{
	if(SUBTYPE==SUBTYPE_HD63705)
	{
		BRANCH( m6805.nmi_state!=CLEAR_LINE );
	}
	else
	{
		BRANCH( m6805.irq_state[0]!=CLEAR_LINE );
	}
}

/* $2f BIH relative ---- */
static INLINE void bih( void )
{
	if(SUBTYPE==SUBTYPE_HD63705)
	{
		BRANCH( m6805.nmi_state==CLEAR_LINE );
	}
	else
	{
		BRANCH( m6805.irq_state[0]==CLEAR_LINE );
	}
}


#ifdef macintosh
#pragma mark ____3x____
#endif

/* $30 NEG direct -*** */
static INLINE void neg_di( void )
{
	UINT8 t;
	UINT16 r;
	DIRBYTE(t); r=-t;
	CLR_NZC; SET_FLAGS8(0,t,r);
	WM(EAD,r);
}

/* $31 ILLEGAL */

/* $32 ILLEGAL */

/* $33 COM direct -**1 */
static INLINE void com_di( void )
{
	UINT8 t;
	DIRBYTE(t); t = ~t;
	CLR_NZ; SET_NZ8(t); SEC;
	WM(EAD,t);
}

/* $34 LSR direct -0** */
static INLINE void lsr_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	CLR_NZC;
	CC |= (t&0x01);
	t >>= 1;
	SET_Z8(t);
	WM(EAD,t);
}

/* $35 ILLEGAL */

/* $36 ROR direct -*** */
static INLINE void ror_di( void )
{
	UINT8 t,r;
	DIRBYTE(t);
	r = (CC & 0x01) << 7;
	CLR_NZC;
	CC |= (t & 0x01);
	r |= t>>1;
	SET_NZ8(r);
	WM(EAD,r);
}

/* $37 ASR direct ?*** */
static INLINE void asr_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	CLR_NZC; CC|=(t&0x01);
	t>>=1; t|=((t&0x40)<<1);
	SET_NZ8(t);
	WM(EAD,t);
}

/* $38 LSL direct ?*** */
static INLINE void lsl_di( void )
{
	UINT8 t;
	UINT16 r;
	DIRBYTE(t);
	r = t << 1;
	CLR_NZC;
	SET_FLAGS8(t,t,r);
	WM(EAD,r);
}

/* $39 ROL direct -*** */
static INLINE void rol_di( void )
{
	UINT16 t,r;
	DIRBYTE(t);
	r = CC & 0x01;
	r |= t << 1;
	CLR_NZC;
	SET_FLAGS8(t,t,r);
	WM(EAD,r);
}

/* $3a DEC direct -**- */
static INLINE void dec_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	--t;
	CLR_NZ; SET_FLAGS8D(t);
	WM(EAD,t);
}

/* $3b ILLEGAL */

/* $3c INC direct -**- */
static INLINE void inc_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	++t;
	CLR_NZ; SET_FLAGS8I(t);
	WM(EAD,t);
}

/* $3d TST direct -**- */
static INLINE void tst_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	CLR_NZ; SET_NZ8(t);
}

/* $3e ILLEGAL */

/* $3f CLR direct -0100 */
static INLINE void clr_di( void )
{
	DIRECT;
	CLR_NZC; SEZ;
	WM(EAD,0);
}


#ifdef macintosh
#pragma mark ____4x____
#endif

/* $40 NEGA inherent ?*** */
static INLINE void nega( void )
{
	UINT16 r;
	r = -A;
	CLR_NZC; SET_FLAGS8(0,A,r);
	A = r;
}

/* $41 ILLEGAL */

/* $42 ILLEGAL */

/* $43 COMA inherent -**1 */
static INLINE void coma( void )
{
	A = ~A;
	CLR_NZ;
	SET_NZ8(A);
	SEC;
}

/* $44 LSRA inherent -0** */
static INLINE void lsra( void )
{
	CLR_NZC;
	CC |= (A & 0x01);
	A >>= 1;
	SET_Z8(A);
}

/* $45 ILLEGAL */

/* $46 RORA inherent -*** */
static INLINE void rora( void )
{
	UINT8 r;
	r = (CC & 0x01) << 7;
	CLR_NZC;
	CC |= (A & 0x01);
	r |= A >> 1;
	SET_NZ8(r);
	A = r;
}

/* $47 ASRA inherent ?*** */
static INLINE void asra( void )
{
	CLR_NZC;
	CC |= (A & 0x01);
	A = (A & 0x80) | (A >> 1);
	SET_NZ8(A);
}

/* $48 LSLA inherent ?*** */
static INLINE void lsla( void )
{
	UINT16 r;
	r = A << 1;
	CLR_NZC;
	SET_FLAGS8(A,A,r);
	A = r;
}

/* $49 ROLA inherent -*** */
static INLINE void rola( void )
{
	UINT16 t,r;
	t = A;
	r = CC & 0x01;
	r |= t << 1;
	CLR_NZC;
	SET_FLAGS8(t,t,r);
	A = r;
}

/* $4a DECA inherent -**- */
static INLINE void deca( void )
{
	--A;
	CLR_NZ;
	SET_FLAGS8D(A);
}

/* $4b ILLEGAL */

/* $4c INCA inherent -**- */
static INLINE void inca( void )
{
	++A;
	CLR_NZ;
	SET_FLAGS8I(A);
}

/* $4d TSTA inherent -**- */
static INLINE void tsta( void )
{
	CLR_NZ;
	SET_NZ8(A);
}

/* $4e ILLEGAL */

/* $4f CLRA inherent -010 */
static INLINE void clra( void )
{
	A = 0;
	CLR_NZC;
	SEZ;
}


#ifdef macintosh
#pragma mark ____5x____
#endif

/* $50 NEGX inherent ?*** */
static INLINE void negx( void )
{
	UINT16 r;
	r = -X;
	CLR_NZC;
	SET_FLAGS8(0,X,r);
	X = r;
}

/* $51 ILLEGAL */

/* $52 ILLEGAL */

/* $53 COMX inherent -**1 */
static INLINE void comx( void )
{
	X = ~X;
	CLR_NZ;
	SET_NZ8(X);
	SEC;
}

/* $54 LSRX inherent -0** */
static INLINE void lsrx( void )
{
	CLR_NZC;
	CC |= (X & 0x01);
	X >>= 1;
	SET_Z8(X);
}

/* $55 ILLEGAL */

/* $56 RORX inherent -*** */
static INLINE void rorx( void )
{
	UINT8 r;
	r = (CC & 0x01) << 7;
	CLR_NZC;
	CC |= (X & 0x01);
	r |= X>>1;
	SET_NZ8(r);
	X = r;
}

/* $57 ASRX inherent ?*** */
static INLINE void asrx( void )
{
	CLR_NZC;
	CC |= (X & 0x01);
	X = (X & 0x80) | (X >> 1);
	SET_NZ8(X);
}

/* $58 ASLX inherent ?*** */
static INLINE void aslx( void )
{
	UINT16 r;
	r = X << 1;
	CLR_NZC;
	SET_FLAGS8(X,X,r);
	X = r;
}

/* $59 ROLX inherent -*** */
static INLINE void rolx( void )
{
	UINT16 t,r;
	t = X;
	r = CC & 0x01;
	r |= t<<1;
	CLR_NZC;
	SET_FLAGS8(t,t,r);
	X = r;
}

/* $5a DECX inherent -**- */
static INLINE void decx( void )
{
	--X;
	CLR_NZ;
	SET_FLAGS8D(X);
}

/* $5b ILLEGAL */

/* $5c INCX inherent -**- */
static INLINE void incx( void )
{
	++X;
	CLR_NZ;
	SET_FLAGS8I(X);
}

/* $5d TSTX inherent -**- */
static INLINE void tstx( void )
{
	CLR_NZ;
	SET_NZ8(X);
}

/* $5e ILLEGAL */

/* $5f CLRX inherent -010 */
static INLINE void clrx( void )
{
	X = 0;
	CLR_NZC;
	SEZ;
}


#ifdef macintosh
#pragma mark ____6x____
#endif

/* $60 NEG indexed, 1 byte offset -*** */
static INLINE void neg_ix1( void )
{
	UINT8 t;
	UINT16 r;
	IDX1BYTE(t); r=-t;
	CLR_NZC; SET_FLAGS8(0,t,r);
	WM(EAD,r);
}

/* $61 ILLEGAL */

/* $62 ILLEGAL */

/* $63 COM indexed, 1 byte offset -**1 */
static INLINE void com_ix1( void )
{
	UINT8 t;
	IDX1BYTE(t); t = ~t;
	CLR_NZ; SET_NZ8(t); SEC;
	WM(EAD,t);
}

/* $64 LSR indexed, 1 byte offset -0** */
static INLINE void lsr_ix1( void )
{
	UINT8 t;
	IDX1BYTE(t);
	CLR_NZC;
	CC |= (t & 0x01);
	t >>= 1;
	SET_Z8(t);
	WM(EAD,t);
}

/* $65 ILLEGAL */

/* $66 ROR indexed, 1 byte offset -*** */
static INLINE void ror_ix1( void )
{
	UINT8 t,r;
	IDX1BYTE(t);
	r = (CC & 0x01) << 7;
	CLR_NZC;
	CC |= (t & 0x01);
	r |= t>>1;
	SET_NZ8(r);
	WM(EAD,r);
}

/* $67 ASR indexed, 1 byte offset ?*** */
static INLINE void asr_ix1( void )
{
	UINT8 t;
	IDX1BYTE(t);
	CLR_NZC; CC|=(t&0x01);
	t>>=1; t|=((t&0x40)<<1);
	SET_NZ8(t);
	WM(EAD,t);
}

/* $68 LSL indexed, 1 byte offset ?*** */
static INLINE void lsl_ix1( void )
{
	UINT8 t;
	UINT16 r;
	IDX1BYTE(t);
	r = t << 1;
	CLR_NZC;
	SET_FLAGS8(t,t,r);
	WM(EAD,r);
}

/* $69 ROL indexed, 1 byte offset -*** */
static INLINE void rol_ix1( void )
{
	UINT16 t,r;
	IDX1BYTE(t);
	r = CC & 0x01;
	r |= t << 1;
	CLR_NZC;
	SET_FLAGS8(t,t,r);
	WM(EAD,r);
}

/* $6a DEC indexed, 1 byte offset -**- */
static INLINE void dec_ix1( void )
{
	UINT8 t;
	IDX1BYTE(t);
	--t;
	CLR_NZ; SET_FLAGS8D(t);
	WM(EAD,t);
}

/* $6b ILLEGAL */

/* $6c INC indexed, 1 byte offset -**- */
static INLINE void inc_ix1( void )
{
	UINT8 t;
	IDX1BYTE(t);
	++t;
	CLR_NZ; SET_FLAGS8I(t);
	WM(EAD,t);
}

/* $6d TST indexed, 1 byte offset -**- */
static INLINE void tst_ix1( void )
{
	UINT8 t;
	IDX1BYTE(t);
	CLR_NZ; SET_NZ8(t);
}

/* $6e ILLEGAL */

/* $6f CLR indexed, 1 byte offset -0100 */
static INLINE void clr_ix1( void )
{
	INDEXED1;
	CLR_NZC; SEZ;
	WM(EAD,0);
}


#ifdef macintosh
#pragma mark ____7x____
#endif

/* $70 NEG indexed -*** */
static INLINE void neg_ix( void )
{
	UINT8 t;
	UINT16 r;
	IDXBYTE(t); r=-t;
	CLR_NZC; SET_FLAGS8(0,t,r);
	WM(EAD,r);
}

/* $71 ILLEGAL */

/* $72 ILLEGAL */

/* $73 COM indexed -**1 */
static INLINE void com_ix( void )
{
	UINT8 t;
	IDXBYTE(t); t = ~t;
	CLR_NZ; SET_NZ8(t); SEC;
	WM(EAD,t);
}

/* $74 LSR indexed -0** */
static INLINE void lsr_ix( void )
{
	UINT8 t;
	IDXBYTE(t);
	CLR_NZC;
	CC |= (t & 0x01);
	t >>= 1;
	SET_Z8(t);
	WM(EAD,t);
}

/* $75 ILLEGAL */

/* $76 ROR indexed -*** */
static INLINE void ror_ix( void )
{
	UINT8 t,r;
	IDXBYTE(t);
	r = (CC & 0x01) << 7;
	CLR_NZC;
	CC |= (t & 0x01);
	r |= t >> 1;
	SET_NZ8(r);
	WM(EAD,r);
}

/* $77 ASR indexed ?*** */
static INLINE void asr_ix( void )
{
	UINT8 t;
	IDXBYTE(t);
	CLR_NZC;
	CC |= (t & 0x01);
	t = (t & 0x80) | (t >> 1);
	SET_NZ8(t);
	WM(EAD,t);
}

/* $78 LSL indexed ?*** */
static INLINE void lsl_ix( void )
{
	UINT8 t;
	UINT16 r;
	IDXBYTE(t); r=t<<1;
	CLR_NZC; SET_FLAGS8(t,t,r);
	WM(EAD,r);
}

/* $79 ROL indexed -*** */
static INLINE void rol_ix( void )
{
	UINT16 t,r;
	IDXBYTE(t);
	r = CC & 0x01;
	r |= t << 1;
	CLR_NZC;
	SET_FLAGS8(t,t,r);
	WM(EAD,r);
}

/* $7a DEC indexed -**- */
static INLINE void dec_ix( void )
{
	UINT8 t;
	IDXBYTE(t);
	--t;
	CLR_NZ; SET_FLAGS8D(t);
	WM(EAD,t);
}

/* $7b ILLEGAL */

/* $7c INC indexed -**- */
static INLINE void inc_ix( void )
{
	UINT8 t;
	IDXBYTE(t);
	++t;
	CLR_NZ; SET_FLAGS8I(t);
	WM(EAD,t);
}

/* $7d TST indexed -**- */
static INLINE void tst_ix( void )
{
	UINT8 t;
	IDXBYTE(t);
	CLR_NZ; SET_NZ8(t);
}

/* $7e ILLEGAL */

/* $7f CLR indexed -0100 */
static INLINE void clr_ix( void )
{
	INDEXED;
	CLR_NZC; SEZ;
	WM(EAD,0);
}


#ifdef macintosh
#pragma mark ____8x____
#endif

/* $80 RTI inherent #### */
static INLINE void rti( void )
{
	PULLBYTE(CC);
	PULLBYTE(A);
	PULLBYTE(X);
	PULLWORD(pPC);
	PC &= m6805.amask;
#if IRQ_LEVEL_DETECT
	if( m6805.irq_state != CLEAR_LINE && (CC & IFLAG) == 0 )
		m6805.pending_interrupts |= M6805_INT_IRQ;
#endif
}

/* $81 RTS inherent ---- */
static INLINE void rts( void )
{
	PULLWORD(pPC);
	PC &= m6805.amask;
}

/* $82 ILLEGAL */

/* $83 SWI absolute indirect ---- */
static INLINE void swi( void )
{
	PUSHWORD(m6805.pc);
	PUSHBYTE(m6805.x);
	PUSHBYTE(m6805.a);
	PUSHBYTE(m6805.cc);
	SEI;
	if(SUBTYPE==SUBTYPE_HD63705) RM16( 0x1ffa, &pPC ); else RM16( AMASK - 3, &pPC );
}

/* $84 ILLEGAL */

/* $85 ILLEGAL */

/* $86 ILLEGAL */

/* $87 ILLEGAL */

/* $88 ILLEGAL */

/* $89 ILLEGAL */

/* $8A ILLEGAL */

/* $8B ILLEGAL */

/* $8C ILLEGAL */

/* $8D ILLEGAL */

/* $8E ILLEGAL */

/* $8F ILLEGAL */


#ifdef macintosh
#pragma mark ____9x____
#endif

/* $90 ILLEGAL */

/* $91 ILLEGAL */

/* $92 ILLEGAL */

/* $93 ILLEGAL */

/* $94 ILLEGAL */

/* $95 ILLEGAL */

/* $96 ILLEGAL */

/* $97 TAX inherent ---- */
static INLINE void tax (void)
{
	X = A;
}

/* $98 CLC */

/* $99 SEC */

/* $9A CLI */

/* $9B SEI */

/* $9C RSP inherent ---- */
static INLINE void rsp (void)
{
	S = SP_MASK;
}

/* $9D NOP inherent ---- */
static INLINE void nop (void)
{
}

/* $9E ILLEGAL */

/* $9F TXA inherent ---- */
static INLINE void txa (void)
{
	A = X;
}


#ifdef macintosh
#pragma mark ____Ax____
#endif

/* $a0 SUBA immediate ?*** */
static INLINE void suba_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = A - t;
	CLR_NZC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $a1 CMPA immediate ?*** */
static INLINE void cmpa_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = A - t;
	CLR_NZC;
	SET_FLAGS8(A,t,r);
}

/* $a2 SBCA immediate ?*** */
static INLINE void sbca_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = A - t - (CC & 0x01);
	CLR_NZC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $a3 CPX immediate -*** */
static INLINE void cpx_im( void )
{
	UINT16	  t,r;
	IMMBYTE(t);
	r = X - t;
	CLR_NZC;
	SET_FLAGS8(X,t,r);
}

/* $a4 ANDA immediate -**- */
static INLINE void anda_im( void )
{
	UINT8 t;
	IMMBYTE(t);
	A &= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $a5 BITA immediate -**- */
static INLINE void bita_im( void )
{
	UINT8 t,r;
	IMMBYTE(t);
	r = A & t;
	CLR_NZ;
	SET_NZ8(r);
}

/* $a6 LDA immediate -**- */
static INLINE void lda_im( void )
{
	IMMBYTE(A);
	CLR_NZ;
	SET_NZ8(A);
}

/* $a7 ILLEGAL */

/* $a8 EORA immediate -**- */
static INLINE void eora_im( void )
{
	UINT8 t;
	IMMBYTE(t);
	A ^= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $a9 ADCA immediate **** */
static INLINE void adca_im( void )
{
	UINT16 t,r;
	IMMBYTE(t);
	r = A + t + (CC & 0x01);
	CLR_HNZC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $aa ORA immediate -**- */
static INLINE void ora_im( void )
{
	UINT8 t;
	IMMBYTE(t);
	A |= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $ab ADDA immediate **** */
static INLINE void adda_im( void )
{
	UINT16 t,r;
	IMMBYTE(t);
	r = A + t;
	CLR_HNZC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $ac ILLEGAL */

/* $ad BSR ---- */
static INLINE void bsr( void )
{
	UINT8 t;
	IMMBYTE(t);
	PUSHWORD(m6805.pc);
	PC += SIGNED(t);
}

/* $ae LDX immediate -**- */
static INLINE void ldx_im( void )
{
	IMMBYTE(X);
	CLR_NZ;
	SET_NZ8(X);
}

/* $af ILLEGAL */


#ifdef macintosh
#pragma mark ____Bx____
#endif

/* $b0 SUBA direct ?*** */
static INLINE void suba_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = A - t;
	CLR_NZC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $b1 CMPA direct ?*** */
static INLINE void cmpa_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = A - t;
	CLR_NZC;
	SET_FLAGS8(A,t,r);
}

/* $b2 SBCA direct ?*** */
static INLINE void sbca_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = A - t - (CC & 0x01);
	CLR_NZC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $b3 CPX direct -*** */
static INLINE void cpx_di( void )
{
	UINT16	  t,r;
	DIRBYTE(t);
	r = X - t;
	CLR_NZC;
	SET_FLAGS8(X,t,r);
}

/* $b4 ANDA direct -**- */
static INLINE void anda_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	A &= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $b5 BITA direct -**- */
static INLINE void bita_di( void )
{
	UINT8 t,r;
	DIRBYTE(t);
	r = A & t;
	CLR_NZ;
	SET_NZ8(r);
}

/* $b6 LDA direct -**- */
static INLINE void lda_di( void )
{
	DIRBYTE(A);
	CLR_NZ;
	SET_NZ8(A);
}

/* $b7 STA direct -**- */
static INLINE void sta_di( void )
{
	CLR_NZ;
	SET_NZ8(A);
	DIRECT;
	WM(EAD,A);
}

/* $b8 EORA direct -**- */
static INLINE void eora_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	A ^= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $b9 ADCA direct **** */
static INLINE void adca_di( void )
{
	UINT16 t,r;
	DIRBYTE(t);
	r = A + t + (CC & 0x01);
	CLR_HNZC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $ba ORA direct -**- */
static INLINE void ora_di( void )
{
	UINT8 t;
	DIRBYTE(t);
	A |= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $bb ADDA direct **** */
static INLINE void adda_di( void )
{
	UINT16 t,r;
	DIRBYTE(t);
	r = A + t;
	CLR_HNZC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $bc JMP direct -*** */
static INLINE void jmp_di( void )
{
	DIRECT;
	PC = EA;
}

/* $bd JSR direct ---- */
static INLINE void jsr_di( void )
{
	DIRECT;
	PUSHWORD(m6805.pc);
	PC = EA;
}

/* $be LDX direct -**- */
static INLINE void ldx_di( void )
{
	DIRBYTE(X);
	CLR_NZ;
	SET_NZ8(X);
}

/* $bf STX direct -**- */
static INLINE void stx_di( void )
{
	CLR_NZ;
	SET_NZ8(X);
	DIRECT;
	WM(EAD,X);
}


#ifdef macintosh
#pragma mark ____Cx____
#endif

/* $c0 SUBA extended ?*** */
static INLINE void suba_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = A - t;
	CLR_NZC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $c1 CMPA extended ?*** */
static INLINE void cmpa_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = A - t;
	CLR_NZC;
	SET_FLAGS8(A,t,r);
}

/* $c2 SBCA extended ?*** */
static INLINE void sbca_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = A - t - (CC & 0x01);
	CLR_NZC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $c3 CPX extended -*** */
static INLINE void cpx_ex( void )
{
	UINT16	  t,r;
	EXTBYTE(t);
	r = X - t;
	CLR_NZC;
	SET_FLAGS8(X,t,r);
}

/* $c4 ANDA extended -**- */
static INLINE void anda_ex( void )
{
	UINT8 t;
	EXTBYTE(t);
	A &= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $c5 BITA extended -**- */
static INLINE void bita_ex( void )
{
	UINT8 t,r;
	EXTBYTE(t);
	r = A & t;
	CLR_NZ;
	SET_NZ8(r);
}

/* $c6 LDA extended -**- */
static INLINE void lda_ex( void )
{
	EXTBYTE(A);
	CLR_NZ;
	SET_NZ8(A);
}

/* $c7 STA extended -**- */
static INLINE void sta_ex( void )
{
	CLR_NZ;
	SET_NZ8(A);
	EXTENDED;
	WM(EAD,A);
}

/* $c8 EORA extended -**- */
static INLINE void eora_ex( void )
{
	UINT8 t;
	EXTBYTE(t);
	A ^= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $c9 ADCA extended **** */
static INLINE void adca_ex( void )
{
	UINT16 t,r;
	EXTBYTE(t);
	r = A + t + (CC & 0x01);
	CLR_HNZC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $ca ORA extended -**- */
static INLINE void ora_ex( void )
{
	UINT8 t;
	EXTBYTE(t);
	A |= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $cb ADDA extended **** */
static INLINE void adda_ex( void )
{
	UINT16 t,r;
	EXTBYTE(t);
	r = A + t;
	CLR_HNZC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $cc JMP extended -*** */
static INLINE void jmp_ex( void )
{
	EXTENDED;
	PC = EA;
}

/* $cd JSR extended ---- */
static INLINE void jsr_ex( void )
{
	EXTENDED;
	PUSHWORD(m6805.pc);
	PC = EA;
}

/* $ce LDX extended -**- */
static INLINE void ldx_ex( void )
{
	EXTBYTE(X);
	CLR_NZ;
	SET_NZ8(X);
}

/* $cf STX extended -**- */
static INLINE void stx_ex( void )
{
	CLR_NZ;
	SET_NZ8(X);
	EXTENDED;
	WM(EAD,X);
}


#ifdef macintosh
#pragma mark ____Dx____
#endif

/* $d0 SUBA indexed, 2 byte offset ?*** */
static INLINE void suba_ix2( void )
{
	UINT16	  t,r;
	IDX2BYTE(t);
	r = A - t;
	CLR_NZC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $d1 CMPA indexed, 2 byte offset ?*** */
static INLINE void cmpa_ix2( void )
{
	UINT16	  t,r;
	IDX2BYTE(t);
	r = A - t;
	CLR_NZC;
	SET_FLAGS8(A,t,r);
}

/* $d2 SBCA indexed, 2 byte offset ?*** */
static INLINE void sbca_ix2( void )
{
	UINT16	  t,r;
	IDX2BYTE(t);
	r = A - t - (CC & 0x01);
	CLR_NZC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $d3 CPX indexed, 2 byte offset -*** */
static INLINE void cpx_ix2( void )
{
	UINT16	  t,r;
	IDX2BYTE(t);
	r = X - t;
	CLR_NZC;
	SET_FLAGS8(X,t,r);
}

/* $d4 ANDA indexed, 2 byte offset -**- */
static INLINE void anda_ix2( void )
{
	UINT8 t;
	IDX2BYTE(t);
	A &= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $d5 BITA indexed, 2 byte offset -**- */
static INLINE void bita_ix2( void )
{
	UINT8 t,r;
	IDX2BYTE(t);
	r = A & t;
	CLR_NZ;
	SET_NZ8(r);
}

/* $d6 LDA indexed, 2 byte offset -**- */
static INLINE void lda_ix2( void )
{
	IDX2BYTE(A);
	CLR_NZ;
	SET_NZ8(A);
}

/* $d7 STA indexed, 2 byte offset -**- */
static INLINE void sta_ix2( void )
{
	CLR_NZ;
	SET_NZ8(A);
	INDEXED2;
	WM(EAD,A);
}

/* $d8 EORA indexed, 2 byte offset -**- */
static INLINE void eora_ix2( void )
{
	UINT8 t;
	IDX2BYTE(t);
	A ^= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $d9 ADCA indexed, 2 byte offset **** */
static INLINE void adca_ix2( void )
{
	UINT16 t,r;
	IDX2BYTE(t);
	r = A + t + (CC & 0x01);
	CLR_HNZC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $da ORA indexed, 2 byte offset -**- */
static INLINE void ora_ix2( void )
{
	UINT8 t;
	IDX2BYTE(t);
	A |= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $db ADDA indexed, 2 byte offset **** */
static INLINE void adda_ix2( void )
{
	UINT16 t,r;
	IDX2BYTE(t);
	r = A + t;
	CLR_HNZC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $dc JMP indexed, 2 byte offset -*** */
static INLINE void jmp_ix2( void )
{
	INDEXED2;
	PC = EA;
}

/* $dd JSR indexed, 2 byte offset ---- */
static INLINE void jsr_ix2( void )
{
	INDEXED2;
	PUSHWORD(m6805.pc);
	PC = EA;
}

/* $de LDX indexed, 2 byte offset -**- */
static INLINE void ldx_ix2( void )
{
	IDX2BYTE(X);
	CLR_NZ;
	SET_NZ8(X);
}

/* $df STX indexed, 2 byte offset -**- */
static INLINE void stx_ix2( void )
{
	CLR_NZ;
	SET_NZ8(X);
	INDEXED2;
	WM(EAD,X);
}


#ifdef macintosh
#pragma mark ____Ex____
#endif

/* $e0 SUBA indexed, 1 byte offset ?*** */
static INLINE void suba_ix1( void )
{
	UINT16	  t,r;
	IDX1BYTE(t);
	r = A - t;
	CLR_NZC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $e1 CMPA indexed, 1 byte offset ?*** */
static INLINE void cmpa_ix1( void )
{
	UINT16	  t,r;
	IDX1BYTE(t);
	r = A - t;
	CLR_NZC;
	SET_FLAGS8(A,t,r);
}

/* $e2 SBCA indexed, 1 byte offset ?*** */
static INLINE void sbca_ix1( void )
{
	UINT16	  t,r;
	IDX1BYTE(t);
	r = A - t - (CC & 0x01);
	CLR_NZC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $e3 CPX indexed, 1 byte offset -*** */
static INLINE void cpx_ix1( void )
{
	UINT16	  t,r;
	IDX1BYTE(t);
	r = X - t;
	CLR_NZC;
	SET_FLAGS8(X,t,r);
}

/* $e4 ANDA indexed, 1 byte offset -**- */
static INLINE void anda_ix1( void )
{
	UINT8 t;
	IDX1BYTE(t);
	A &= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $e5 BITA indexed, 1 byte offset -**- */
static INLINE void bita_ix1( void )
{
	UINT8 t,r;
	IDX1BYTE(t);
	r = A & t;
	CLR_NZ;
	SET_NZ8(r);
}

/* $e6 LDA indexed, 1 byte offset -**- */
static INLINE void lda_ix1( void )
{
	IDX1BYTE(A);
	CLR_NZ;
	SET_NZ8(A);
}

/* $e7 STA indexed, 1 byte offset -**- */
static INLINE void sta_ix1( void )
{
	CLR_NZ;
	SET_NZ8(A);
	INDEXED1;
	WM(EAD,A);
}

/* $e8 EORA indexed, 1 byte offset -**- */
static INLINE void eora_ix1( void )
{
	UINT8 t;
	IDX1BYTE(t);
	A ^= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $e9 ADCA indexed, 1 byte offset **** */
static INLINE void adca_ix1( void )
{
	UINT16 t,r;
	IDX1BYTE(t);
	r = A + t + (CC & 0x01);
	CLR_HNZC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $ea ORA indexed, 1 byte offset -**- */
static INLINE void ora_ix1( void )
{
	UINT8 t;
	IDX1BYTE(t);
	A |= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $eb ADDA indexed, 1 byte offset **** */
static INLINE void adda_ix1( void )
{
	UINT16 t,r;
	IDX1BYTE(t);
	r = A + t;
	CLR_HNZC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $ec JMP indexed, 1 byte offset -*** */
static INLINE void jmp_ix1( void )
{
	INDEXED1;
	PC = EA;
}

/* $ed JSR indexed, 1 byte offset ---- */
static INLINE void jsr_ix1( void )
{
	INDEXED1;
	PUSHWORD(m6805.pc);
	PC = EA;
}

/* $ee LDX indexed, 1 byte offset -**- */
static INLINE void ldx_ix1( void )
{
	IDX1BYTE(X);
	CLR_NZ;
	SET_NZ8(X);
}

/* $ef STX indexed, 1 byte offset -**- */
static INLINE void stx_ix1( void )
{
	CLR_NZ;
	SET_NZ8(X);
	INDEXED1;
	WM(EAD,X);
}


#ifdef macintosh
#pragma mark ____Fx____
#endif

/* $f0 SUBA indexed ?*** */
static INLINE void suba_ix( void )
{
	UINT16	  t,r;
	IDXBYTE(t);
	r = A - t;
	CLR_NZC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $f1 CMPA indexed ?*** */
static INLINE void cmpa_ix( void )
{
	UINT16	  t,r;
	IDXBYTE(t);
	r = A - t;
	CLR_NZC;
	SET_FLAGS8(A,t,r);
}

/* $f2 SBCA indexed ?*** */
static INLINE void sbca_ix( void )
{
	UINT16	  t,r;
	IDXBYTE(t);
	r = A - t - (CC & 0x01);
	CLR_NZC;
	SET_FLAGS8(A,t,r);
	A = r;
}

/* $f3 CPX indexed -*** */
static INLINE void cpx_ix( void )
{
	UINT16	  t,r;
	IDXBYTE(t);
	r = X - t;
	CLR_NZC;
	SET_FLAGS8(X,t,r);
}

/* $f4 ANDA indexed -**- */
static INLINE void anda_ix( void )
{
	UINT8 t;
	IDXBYTE(t);
	A &= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $f5 BITA indexed -**- */
static INLINE void bita_ix( void )
{
	UINT8 t,r;
	IDXBYTE(t);
	r = A & t;
	CLR_NZ;
	SET_NZ8(r);
}

/* $f6 LDA indexed -**- */
static INLINE void lda_ix( void )
{
	IDXBYTE(A);
	CLR_NZ;
	SET_NZ8(A);
}

/* $f7 STA indexed -**- */
static INLINE void sta_ix( void )
{
	CLR_NZ;
	SET_NZ8(A);
	INDEXED;
	WM(EAD,A);
}

/* $f8 EORA indexed -**- */
static INLINE void eora_ix( void )
{
	UINT8 t;
	IDXBYTE(t);
	A ^= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $f9 ADCA indexed **** */
static INLINE void adca_ix( void )
{
	UINT16 t,r;
	IDXBYTE(t);
	r = A + t + (CC & 0x01);
	CLR_HNZC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $fa ORA indexed -**- */
static INLINE void ora_ix( void )
{
	UINT8 t;
	IDXBYTE(t);
	A |= t;
	CLR_NZ;
	SET_NZ8(A);
}

/* $fb ADDA indexed **** */
static INLINE void adda_ix( void )
{
	UINT16 t,r;
	IDXBYTE(t);
	r = A + t;
	CLR_HNZC;
	SET_FLAGS8(A,t,r);
	SET_H(A,t,r);
	A = r;
}

/* $fc JMP indexed -*** */
static INLINE void jmp_ix( void )
{
	INDEXED;
	PC = EA;
}

/* $fd JSR indexed ---- */
static INLINE void jsr_ix( void )
{
	INDEXED;
	PUSHWORD(m6805.pc);
	PC = EA;
}

/* $fe LDX indexed -**- */
static INLINE void ldx_ix( void )
{
	IDXBYTE(X);
	CLR_NZ;
	SET_NZ8(X);
}

/* $ff STX indexed -**- */
static INLINE void stx_ix( void )
{
	CLR_NZ;
	SET_NZ8(X);
	INDEXED;
	WM(EAD,X);
}
