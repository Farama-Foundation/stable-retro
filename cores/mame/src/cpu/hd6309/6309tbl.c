static INLINE void illegal( void );
static INLINE void neg_di( void );
static INLINE void oim_di( void );
static INLINE void aim_di( void );
static INLINE void com_di( void );
static INLINE void lsr_di( void );
static INLINE void eim_di( void );
static INLINE void ror_di( void );
static INLINE void asr_di( void );
static INLINE void asl_di( void );
static INLINE void rol_di( void );
static INLINE void dec_di( void );
static INLINE void tim_di( void );
static INLINE void inc_di( void );
static INLINE void tst_di( void );
static INLINE void jmp_di( void );
static INLINE void clr_di( void );
static INLINE void nop( void );
static INLINE void sync( void );
static INLINE void sexw( void );
static INLINE void lbra( void );
static INLINE void lbsr( void );
static INLINE void daa( void );
static INLINE void daa( void );
static INLINE void orcc( void );
static INLINE void andcc( void );
static INLINE void sex( void );
static INLINE void exg( void );
static INLINE void tfr( void );
static INLINE void bra( void );
static INLINE void brn( void );
static INLINE void lbrn( void );
static INLINE void bhi( void );
static INLINE void lbhi( void );
static INLINE void bls( void );
static INLINE void lbls( void );
static INLINE void bcc( void );
static INLINE void lbcc( void );
static INLINE void bcs( void );
static INLINE void lbcs( void );
static INLINE void bne( void );
static INLINE void lbne( void );
static INLINE void beq( void );
static INLINE void lbeq( void );
static INLINE void bvc( void );
static INLINE void lbvc( void );
static INLINE void bvs( void );
static INLINE void lbvs( void );
static INLINE void bpl( void );
static INLINE void lbpl( void );
static INLINE void bmi( void );
static INLINE void lbmi( void );
static INLINE void bge( void );
static INLINE void lbge( void );
static INLINE void blt( void );
static INLINE void lblt( void );
static INLINE void bgt( void );
static INLINE void lbgt( void );
static INLINE void ble( void );
static INLINE void lble( void );
static INLINE void addr_r( void );
static INLINE void adcr( void );
static INLINE void subr( void );
static INLINE void sbcr( void );
static INLINE void andr( void );
static INLINE void orr( void );
static INLINE void eorr( void );
static INLINE void cmpr( void );
static INLINE void tfmpp( void );
static INLINE void tfmmm( void );
static INLINE void tfmpc( void );
static INLINE void tfmcp( void );
static INLINE void bitmd_im( void );
static INLINE void leax( void );
static INLINE void leay( void );
static INLINE void leas( void );
static INLINE void leau( void );
static INLINE void pshs( void );
static INLINE void ldmd_im( void );
static INLINE void pshsw( void );
static INLINE void pshuw( void );
static INLINE void puls( void );
static INLINE void pulsw( void );
static INLINE void puluw( void );
static INLINE void pshu( void );
static INLINE void pulu( void );
static INLINE void rts( void );
static INLINE void abx( void );
static INLINE void rti( void );
static INLINE void cwai( void );
static INLINE void bitd_di( void );
static INLINE void bitd_ix( void );
static INLINE void bitd_ex( void );
static INLINE void mul( void );
static INLINE void swi( void );
static INLINE void band( void );
static INLINE void bitd_im( void );
static INLINE void biand( void );
static INLINE void bor( void );
static INLINE void bior( void );
static INLINE void beor( void );
static INLINE void bieor( void );
static INLINE void ldbt( void );
static INLINE void stbt( void );
static INLINE void swi2( void );
static INLINE void swi3( void );
static INLINE void nega( void );
static INLINE void coma( void );
static INLINE void lsra( void );
static INLINE void rora( void );
static INLINE void asra( void );
static INLINE void asla( void );
static INLINE void rola( void );
static INLINE void deca( void );
static INLINE void inca( void );
static INLINE void tsta( void );
static INLINE void clra( void );
static INLINE void negb( void );
static INLINE void negd( void );
static INLINE void comb( void );
static INLINE void come( void );
static INLINE void comf( void );
static INLINE void comd( void );
static INLINE void comw( void );
static INLINE void lsrb( void );
static INLINE void lsrd( void );
static INLINE void lsrw( void );
static INLINE void rorb( void );
static INLINE void rord( void );
static INLINE void rorw( void );
static INLINE void asrb( void );
static INLINE void asrd( void );
static INLINE void aslb( void );
static INLINE void asld( void );
static INLINE void rolb( void );
static INLINE void rold( void );
static INLINE void rolw( void );
static INLINE void decb( void );
static INLINE void dece( void );
static INLINE void decf( void );
static INLINE void decd( void );
static INLINE void decw( void );
static INLINE void incb( void );
static INLINE void ince( void );
static INLINE void incf( void );
static INLINE void incd( void );
static INLINE void incw( void );
static INLINE void tstb( void );
static INLINE void tstd( void );
static INLINE void tstw( void );
static INLINE void tste( void );
static INLINE void tstf( void );
static INLINE void clrb( void );
static INLINE void clrd( void );
static INLINE void clre( void );
static INLINE void clrf( void );
static INLINE void clrw( void );
static INLINE void neg_ix( void );
static INLINE void oim_ix( void );
static INLINE void aim_ix( void );
static INLINE void com_ix( void );
static INLINE void lsr_ix( void );
static INLINE void eim_ix( void );
static INLINE void ror_ix( void );
static INLINE void asr_ix( void );
static INLINE void asl_ix( void );
static INLINE void rol_ix( void );
static INLINE void dec_ix( void );
static INLINE void tim_ix( void );
static INLINE void inc_ix( void );
static INLINE void tst_ix( void );
static INLINE void jmp_ix( void );
static INLINE void clr_ix( void );
static INLINE void neg_ex( void );
static INLINE void oim_ex( void );
static INLINE void aim_ex( void );
static INLINE void com_ex( void );
static INLINE void lsr_ex( void );
static INLINE void eim_ex( void );
static INLINE void ror_ex( void );
static INLINE void asr_ex( void );
static INLINE void asl_ex( void );
static INLINE void rol_ex( void );
static INLINE void dec_ex( void );
static INLINE void tim_ex( void );
static INLINE void inc_ex( void );
static INLINE void tst_ex( void );
static INLINE void jmp_ex( void );
static INLINE void clr_ex( void );
static INLINE void suba_im( void );
static INLINE void cmpa_im( void );
static INLINE void sbca_im( void );
static INLINE void subd_im( void );
static INLINE void subw_im( void );
static INLINE void cmpd_im( void );
static INLINE void cmpw_im( void );
static INLINE void cmpu_im( void );
static INLINE void anda_im( void );
static INLINE void bita_im( void );
static INLINE void lda_im( void );
static INLINE void eora_im( void );
static INLINE void adca_im( void );
static INLINE void ora_im( void );
static INLINE void adda_im( void );
static INLINE void cmpx_im( void );
static INLINE void cmpy_im( void );
static INLINE void cmps_im( void );
static INLINE void bsr( void );
static INLINE void ldx_im( void );
static INLINE void ldq_im( void );
static INLINE void ldy_im( void );
static INLINE void suba_di( void );
static INLINE void cmpa_di( void );
static INLINE void sbca_di( void );
static INLINE void subd_di( void );
static INLINE void subw_di( void );
static INLINE void cmpd_di( void );
static INLINE void cmpw_di( void );
static INLINE void cmpu_di( void );
static INLINE void anda_di( void );
static INLINE void bita_di( void );
static INLINE void lda_di( void );
static INLINE void sta_di( void );
static INLINE void eora_di( void );
static INLINE void adca_di( void );
static INLINE void ora_di( void );
static INLINE void adda_di( void );
static INLINE void cmpx_di( void );
static INLINE void cmpy_di( void );
static INLINE void cmps_di( void );
static INLINE void jsr_di( void );
static INLINE void ldx_di( void );
static INLINE void muld_di( void );
static INLINE void divd_im( void );
static INLINE void divq_im( void );
static INLINE void muld_im( void );
static INLINE void divd_di( void );
static INLINE void divq_di( void );
static INLINE void ldq_di( void );
static INLINE void ldy_di( void );
static INLINE void stx_di( void );
static INLINE void stq_di( void );
static INLINE void sty_di( void );
static INLINE void suba_ix( void );
static INLINE void cmpa_ix( void );
static INLINE void sbca_ix( void );
static INLINE void subd_ix( void );
static INLINE void subw_ix( void );
static INLINE void cmpd_ix( void );
static INLINE void cmpw_ix( void );
static INLINE void cmpu_ix( void );
static INLINE void anda_ix( void );
static INLINE void bita_ix( void );
static INLINE void lda_ix( void );
static INLINE void sta_ix( void );
static INLINE void eora_ix( void );
static INLINE void adca_ix( void );
static INLINE void ora_ix( void );
static INLINE void adda_ix( void );
static INLINE void cmpx_ix( void );
static INLINE void cmpy_ix( void );
static INLINE void cmps_ix( void );
static INLINE void jsr_ix( void );
static INLINE void ldx_ix( void );
static INLINE void muld_ix( void );
static INLINE void divd_ix( void );
static INLINE void divq_ix( void );
static INLINE void ldq_ix( void );
static INLINE void ldy_ix( void );
static INLINE void stx_ix( void );
static INLINE void stq_ix( void );
static INLINE void sty_ix( void );
static INLINE void suba_ex( void );
static INLINE void cmpa_ex( void );
static INLINE void sbca_ex( void );
static INLINE void subd_ex( void );
static INLINE void subw_ex( void );
static INLINE void cmpd_ex( void );
static INLINE void cmpw_ex( void );
static INLINE void cmpu_ex( void );
static INLINE void anda_ex( void );
static INLINE void bita_ex( void );
static INLINE void lda_ex( void );
static INLINE void sta_ex( void );
static INLINE void eora_ex( void );
static INLINE void adca_ex( void );
static INLINE void ora_ex( void );
static INLINE void adda_ex( void );
static INLINE void cmpx_ex( void );
static INLINE void cmpy_ex( void );
static INLINE void cmps_ex( void );
static INLINE void jsr_ex( void );
static INLINE void ldx_ex( void );
static INLINE void muld_ex( void );
static INLINE void divd_ex( void );
static INLINE void divq_ex( void );
static INLINE void ldq_ex( void );
static INLINE void ldy_ex( void );
static INLINE void stx_ex( void );
static INLINE void stq_ex( void );
static INLINE void sty_ex( void );
static INLINE void subb_im( void );
static INLINE void sube_im( void );
static INLINE void subf_im( void );
static INLINE void cmpb_im( void );
static INLINE void cmpe_im( void );
static INLINE void cmpf_im( void );
static INLINE void sbcb_im( void );
static INLINE void sbcd_im( void );
static INLINE void addd_im( void );
static INLINE void addw_im( void );
static INLINE void adde_im( void );
static INLINE void addf_im( void );
static INLINE void andb_im( void );
static INLINE void andd_im( void );
static INLINE void bitb_im( void );
static INLINE void ldb_im( void );
static INLINE void lde_im( void );
static INLINE void ldf_im( void );
static INLINE void eorb_im( void );
static INLINE void eord_im( void );
static INLINE void adcb_im( void );
static INLINE void adcd_im( void );
static INLINE void orb_im( void );
static INLINE void ord_im( void );
static INLINE void addb_im( void );
static INLINE void ldd_im( void );
static INLINE void ldw_im( void );
static INLINE void ldu_im( void );
static INLINE void lds_im( void );
static INLINE void subb_di( void );
static INLINE void sube_di( void );
static INLINE void subf_di( void );
static INLINE void cmpb_di( void );
static INLINE void cmpe_di( void );
static INLINE void cmpf_di( void );
static INLINE void sbcb_di( void );
static INLINE void sbcd_di( void );
static INLINE void addd_di( void );
static INLINE void addw_di( void );
static INLINE void adde_di( void );
static INLINE void addf_di( void );
static INLINE void andb_di( void );
static INLINE void andd_di( void );
static INLINE void bitb_di( void );
static INLINE void ldb_di( void );
static INLINE void lde_di( void );
static INLINE void ldf_di( void );
static INLINE void stb_di( void );
static INLINE void ste_di( void );
static INLINE void stf_di( void );
static INLINE void eorb_di( void );
static INLINE void eord_di( void );
static INLINE void adcb_di( void );
static INLINE void adcd_di( void );
static INLINE void orb_di( void );
static INLINE void ord_di( void );
static INLINE void addb_di( void );
static INLINE void ldd_di( void );
static INLINE void ldw_di( void );
static INLINE void std_di( void );
static INLINE void stw_di( void );
static INLINE void ldu_di( void );
static INLINE void lds_di( void );
static INLINE void stu_di( void );
static INLINE void sts_di( void );
static INLINE void subb_ix( void );
static INLINE void sube_ix( void );
static INLINE void subf_ix( void );
static INLINE void cmpb_ix( void );
static INLINE void cmpe_ix( void );
static INLINE void cmpf_ix( void );
static INLINE void sbcb_ix( void );
static INLINE void sbcd_ix( void );
static INLINE void addd_ix( void );
static INLINE void addw_ix( void );
static INLINE void adde_ix( void );
static INLINE void addf_ix( void );
static INLINE void andb_ix( void );
static INLINE void andd_ix( void );
static INLINE void bitb_ix( void );
static INLINE void ldb_ix( void );
static INLINE void lde_ix( void );
static INLINE void ldf_ix( void );
static INLINE void stb_ix( void );
static INLINE void ste_ix( void );
static INLINE void stf_ix( void );
static INLINE void eorb_ix( void );
static INLINE void eord_ix( void );
static INLINE void adcb_ix( void );
static INLINE void adcd_ix( void );
static INLINE void orb_ix( void );
static INLINE void ord_ix( void );
static INLINE void addb_ix( void );
static INLINE void ldd_ix( void );
static INLINE void ldw_ix( void );
static INLINE void std_ix( void );
static INLINE void stw_ix( void );
static INLINE void ldu_ix( void );
static INLINE void lds_ix( void );
static INLINE void stu_ix( void );
static INLINE void sts_ix( void );
static INLINE void subb_ex( void );
static INLINE void sube_ex( void );
static INLINE void subf_ex( void );
static INLINE void cmpb_ex( void );
static INLINE void cmpe_ex( void );
static INLINE void cmpf_ex( void );
static INLINE void sbcb_ex( void );
static INLINE void sbcd_ex( void );
static INLINE void addd_ex( void );
static INLINE void addw_ex( void );
static INLINE void adde_ex( void );
static INLINE void addf_ex( void );
static INLINE void andb_ex( void );
static INLINE void andd_ex( void );
static INLINE void bitb_ex( void );
static INLINE void ldb_ex( void );
static INLINE void lde_ex( void );
static INLINE void ldf_ex( void );
static INLINE void stb_ex( void );
static INLINE void ste_ex( void );
static INLINE void stf_ex( void );
static INLINE void eorb_ex( void );
static INLINE void eord_ex( void );
static INLINE void adcb_ex( void );
static INLINE void adcd_ex( void );
static INLINE void orb_ex( void );
static INLINE void ord_ex( void );
static INLINE void addb_ex( void );
static INLINE void ldd_ex( void );
static INLINE void ldw_ex( void );
static INLINE void std_ex( void );
static INLINE void stw_ex( void );
static INLINE void ldu_ex( void );
static INLINE void lds_ex( void );
static INLINE void stu_ex( void );
static INLINE void sts_ex( void );
static INLINE void pref10( void );
static INLINE void pref11( void );

static UINT8 flags8i[256]=	 /* increment */
{
CC_Z,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
CC_N|CC_V,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N
};
static UINT8 flags8d[256]= /* decrement */
{
CC_Z,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,CC_V,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N
};

static UINT8 index_cycle_em[256] = {        /* Index Loopup cycle counts, emulated 6809 */
/*	         0xX0, 0xX1, 0xX2, 0xX3, 0xX4, 0xX5, 0xX6, 0xX7, 0xX8, 0xX9, 0xXA, 0xXB, 0xXC, 0xXD, 0xXE, 0xXF */

/* 0x0X */      1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
/* 0x1X */      1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
/* 0x2X */      1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
/* 0x3X */      1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
/* 0x4X */      1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
/* 0x5X */      1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
/* 0x6X */      1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
/* 0x7X */      1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
/* 0x8X */      2,    3,    2,    3,    0,    1,    1,    1,    1,    4,    1,    4,    1,    5,    4,    0,
/* 0x9X */      3,    6,    5,    6,    3,    4,    4,    4,    4,    7,    4,    7,    4,    8,    7,    5,
/* 0xAX */      2,    3,    2,    3,    0,    1,    1,    1,    1,    4,    1,    4,    1,    5,    4,    5,
/* 0xBX */      5,    6,    5,    6,    3,    4,    4,    4,    4,    7,    4,    7,    4,    8,    7,    8,
/* 0xCX */      2,    3,    2,    3,    0,    1,    1,    1,    1,    4,    1,    4,    1,    5,    4,    3,
/* 0xDX */      4,    6,    5,    6,    3,    4,    4,    4,    4,    7,    4,    7,    4,    8,    7,    8,
/* 0xEX */      2,    3,    2,    3,    0,    1,    1,    1,    1,    4,    1,    4,    1,    5,    4,    3,
/* 0xFX */      4,    6,    5,    6,    3,    4,    4,    4,    4,    7,    4,    7,    4,    8,    7,    8
};

static UINT8 index_cycle_na[256] = {         /* Index Loopup cycle counts,
native 6309 */
/*	     X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, XA, XB, XC, XD, XE, XF */

/* 0x0X */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 0x1X */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 0x2X */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 0x3X */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 0x4X */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 0x5X */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 0x6X */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 0x7X */   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 0x8X */   1,  2,  1,  2,  0,  1,  1,  1,  1,  3,  1,  2,  1,  3,  1,  0,
/* 0x9X */   3,  5,  4,  5,  3,  4,  4,  4,  4,  7,  4,  5,  4,  6,  4,  5,
/* 0xAX */   1,  2,  1,  2,  0,  1,  1,  1,  1,  3,  1,  2,  1,  3,  1,  2,
/* 0xBX */   5,  5,  4,  5,  3,  4,  4,  4,  4,  7,  4,  5,  4,  6,  4,  5,
/* 0xCX */   1,  2,  1,  2,  0,  1,  1,  1,  1,  3,  1,  2,  1,  3,  1,  1,
/* 0xDX */   4,  5,  4,  5,  3,  4,  4,  4,  4,  7,  4,  5,  4,  6,  4,  5,
/* 0xEX */   1,  2,  1,  2,  0,  1,  1,  1,  1,  3,  1,  2,  1,  3,  1,  1,
/* 0xFX */   4,  5,  4,  5,  3,  4,  4,  4,  4,  7,  4,  5,  4,  6,  4,  5
};

#define IIP0	19			/* Illegal instruction cycle count page 0 */
#define IIP1	20			/* Illegal instruction cycle count page 01 & 11 */

static UINT8 ccounts_page0_em[256] =    /* Cycle Counts Page zero, Emulated 6809 */
{
/*	         0xX0, 0xX1, 0xX2, 0xX3, 0xX4, 0xX5, 0xX6, 0xX7, 0xX8, 0xX9, 0xXA, 0xXB, 0xXC, 0xXD, 0xXE, 0xXF */
/* 0x0X */     6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    3,    6,
/* 0x1X */     0,    0,    2,    4,    2, IIP0,    5,    9, IIP0,    2,    3, IIP0,    3,    2,    8,    6,
/* 0x2X */     3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
/* 0x3X */     4,    4,    4,    4,    5,    5,    5,    5, IIP0,    5,    3,    6,   20,   11, IIP0,   19,
/* 0x4X */     2, IIP0, IIP0,    2,    2, IIP0,    2,    2,    2,    2,    2, IIP0,    2,    2, IIP0,    2,
/* 0x5X */     2, IIP0, IIP0,    2,    2, IIP0,    2,    2,    2,    2,    2, IIP0,    2,    2, IIP0,    2,
/* 0x6X */     6,    7,    7,    6,    6,    6,    6,    6,    6,    6,    6,    7,    6,    6,    3,    6,
/* 0x7X */     7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,    5,    7,    7,    4,    7,
/* 0x8X */     2,    2,    2,    4,    2,    2,    2, IIP0,    2,    2,    2,    2,    4,    7,    3, IIP0,
/* 0x9X */     4,    4,    4,    6,    4,    4,    4,    4,    4,    4,    4,    4,    6,    7,    5,    5,
/* 0xAX */     4,    4,    4,    6,    4,    4,    4,    4,    4,    4,    4,    4,    6,    7,    5,    5,
/* 0xBX */     5,    5,    5,    7,    5,    5,    5,    5,    5,    5,    5,    5,    7,    8,    6,    6,
/* 0xCX */     2,    2,    2,    4,    2,    2,    2, IIP0,    2,    2,    2,    2,    3,    5,    3, IIP0,
/* 0xDX */     4,    4,    4,    6,    4,    4,    4,    4,    4,    4,    4,    4,    5,    5,    5,    5,
/* 0xEX */     4,    4,    4,    6,    4,    4,    4,    4,    4,    4,    4,    4,    5,    5,    5,    5,
/* 0xFX */     5,    5,    5,    7,    5,    5,    5,    5,    5,    5,    5,    5,    6,    6,    6,    6
};

static UINT8 ccounts_page0_na[256] =   /* Cycle Counts Page zero, Native 6309 */
{
/*	         0xX0, 0xX1, 0xX2, 0xX3, 0xX4, 0xX5, 0xX6, 0xX7, 0xX8, 0xX9, 0xXA, 0xXB, 0xXC, 0xXD, 0xXE, 0xXF */
/* 0x0X */     5,    6,    6,    5,    5,    6,    5,    5,    5,    5,    5,    6,    5,    4,    2,    5,
/* 0x1X */     0,    0,    1,    4,    1, IIP0,    4,    7, IIP0,    1,    2, IIP0,    3,    1,    5,    4,
/* 0x2X */     3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
/* 0x3X */     4,    4,    4,    4,    4,    4,    4,    4, IIP0,    4,    1,    6,   22,   10, IIP0,   21,
/* 0x4X */     1, IIP0, IIP0,    1,    1, IIP0,    1,    1,    1,    1,    1, IIP0,    1,    1, IIP0,    1,
/* 0x5X */     1, IIP0, IIP0,    1,    1, IIP0,    1,    1,    1,    1,    1, IIP0,    1,    1, IIP0,    1,
/* 0x6X */     6,    7,    7,    6,    6,    6,    6,    6,    6,    6,    6,    7,    6,    5,    3,    6,
/* 0x7X */     6,    7,    7,    6,    6,    7,    6,    6,    6,    6,    6,    5,    6,    5,    3,    6,
/* 0x8X */     2,    2,    2,    3,    2,    2,    2, IIP0,    2,    2,    2,    2,    3,    6,    3, IIP0,
/* 0x9X */     3,    3,    3,    4,    3,    3,    3,    3,    3,    3,    3,    3,    4,    6,    4,    4,
/* 0xAX */     4,    4,    4,    5,    4,    4,    4,    4,    4,    4,    4,    4,    5,    6,    5,    5,
/* 0xBX */     4,    4,    4,    5,    4,    4,    4,    4,    4,    4,    4,    4,    5,    7,    5,    5,
/* 0xCX */     2,    2,    2,    3,    2,    2,    2, IIP0,    2,    2,    2,    2,    3,    5,    3, IIP0,
/* 0xDX */     3,    3,    3,    4,    3,    3,    3,    3,    3,    3,    3,    3,    4,    4,    4,    4,
/* 0xEX */     4,    4,    4,    5,    4,    4,    4,    4,    4,    4,    4,    4,    5,    5,    5,    5,
/* 0xFX */     4,    4,    4,    5,    4,    4,    4,    4,    4,    4,    4,    4,    5,    5,    5,    5
};

static UINT8 ccounts_page01_em[256] =    /* Cycle Counts Page 01, Emulated 6809 */
{
/*	         0xX0, 0xX1, 0xX2, 0xX3, 0xX4, 0xX5, 0xX6, 0xX7, 0xX8, 0xX9, 0xXA, 0xXB, 0xXC, 0xXD, 0xXE, 0xXF */
/* 0x0X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x1X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x2X */   IIP1,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
/* 0x3X */      4,    4,    4,    4,    4,    4,    4,    4,    6,    6,    6,    6, IIP1, IIP1, IIP1,   20,
/* 0x4X */      2,  IIP1,IIP1,    2,    2, IIP1,    2,    2,    2,    2,    2, IIP1,    2,    2, IIP1,    2,
/* 0x5X */   IIP1, IIP1, IIP1,    3,    3, IIP1,    3, IIP1, IIP1,    3,    3, IIP1,    3,    3, IIP1,    3,
/* 0x6X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x7X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x8X */      5,    5,    5,    5,    5,    5,    4, IIP1,    5,    5,    5,    5,    5, IIP1,    4, IIP1,
/* 0x9X */      7,    7,    7,    7,    7,    7,    6,    6,    7,    7,    7,    7,    7, IIP1,    6,    6,
/* 0xAX */      7,    7,    7,    7,    7,    7,    6,    6,    7,    7,    7,    7,    7, IIP1,    6,    6,
/* 0xBX */      8,    8,    8,    8,    8,    8,    7,    7,    8,    8,    8,    8,    8, IIP1,    7,    7,
/* 0xCX */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,    4, IIP1,
/* 0xDX */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,    8,    8,    6,    6,
/* 0xEX */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,    8,    8,    6,    6,
/* 0xFX */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,    9,    9,    7,    7
};

static UINT8 ccounts_page01_na[256] =   /* Cycle Counts Page 01, Native 6309 */
{
/*	         0xX0, 0xX1, 0xX2, 0xX3, 0xX4, 0xX5, 0xX6, 0xX7, 0xX8, 0xX9, 0xXA, 0xXB, 0xXC, 0xXD, 0xXE, 0xXF */
/* 0x0X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x1X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x2X */   IIP1,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
/* 0x3X */      4,    4,    4,    4,    4,    4,    4,    4,    6,    6,    6,    6, IIP1, IIP1, IIP1,   22,
/* 0x4X */      1, IIP1, IIP1,    1,    1, IIP1,    1,    1,    1,    1,    1, IIP1,    1,    1, IIP1,    1,
/* 0x5X */   IIP1, IIP1, IIP1,    2,    2, IIP1,    2, IIP1, IIP1,    2,    2, IIP1,    2,    2, IIP1,    1,
/* 0x6X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x7X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x8X */      4,    4,    4,    4,    4,    4,    4, IIP1,    4,    4,    4,    4,    4, IIP1,    4, IIP1,
/* 0x9X */      5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5, IIP1,    5,    5,
/* 0xAX */      6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6, IIP1,    6,    6,
/* 0xBX */      6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6, IIP1,    6,    6,
/* 0xCX */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,    4, IIP1,
/* 0xDX */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,    7,    7,    5,    5,
/* 0xEX */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,    8,    8,    6,    6,
/* 0xFX */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,    8,    8,    6,    6
};

static UINT8 ccounts_page11_em[256] =    /* Cycle Counts Page 11, Emulated 6809 */
{
/*	         0xX0, 0xX1, 0xX2, 0xX3, 0xX4, 0xX5, 0xX6, 0xX7, 0xX8, 0xX9, 0xXA, 0xXB, 0xXC, 0xXD, 0xXE, 0xXF */
/* 0x0X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x1X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x2X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x3X */      7,    7,    7,    7,    7,    7,    7,    8,    3,    3,    3,    3,    4,    5, IIP1,   20,
/* 0x4X */   IIP1, IIP1, IIP1,    2, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,    2, IIP1,    2,    2, IIP1,    2,
/* 0x5X */   IIP1, IIP1, IIP1,    2, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,    2, IIP1,    2,    2, IIP1,    2,
/* 0x6X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x7X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x8X */      3,    3, IIP1,    5, IIP1, IIP1,    3, IIP1, IIP1, IIP1, IIP1,    3,    5,   25,   34,   28,
/* 0x9X */      5,    5, IIP1,    7, IIP1, IIP1,    5,    5, IIP1, IIP1, IIP1,    5,    7,   27,   36,   30,
/* 0xAX */      5,    5, IIP1,    7, IIP1, IIP1,    5,    5, IIP1, IIP1, IIP1,    5,    7,   27,   36,   30,
/* 0xBX */      6,    6, IIP1,    8, IIP1, IIP1,    6,    6, IIP1, IIP1, IIP1,    6,    8,   28,   37,   31,
/* 0xCX */      3,    3, IIP1, IIP1, IIP1, IIP1,    3, IIP1, IIP1, IIP1, IIP1,    3, IIP1, IIP1, IIP1, IIP1,
/* 0xDX */      5,    5, IIP1, IIP1, IIP1, IIP1,    5,    5, IIP1, IIP1, IIP1,    5, IIP1, IIP1, IIP1, IIP1,
/* 0xEX */      5,    5, IIP1, IIP1, IIP1, IIP1,    5,    5, IIP1, IIP1, IIP1,    5, IIP1, IIP1, IIP1, IIP1,
/* 0xFX */      6,    6, IIP1, IIP1, IIP1, IIP1,    6,    6, IIP1, IIP1, IIP1,    6, IIP1, IIP1, IIP1, IIP1
};

static UINT8 ccounts_page11_na[256] =    /* Cycle Counts Page 11, Native 6309 */
{
/*	         0xX0, 0xX1, 0xX2, 0xX3, 0xX4, 0xX5, 0xX6, 0xX7, 0xX8, 0xX9, 0xXA, 0xXB, 0xXC, 0xXD, 0xXE, 0xXF */
/* 0x0X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x1X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x2X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x3X */      6,    6,    6,    6,    6,    6,    6,    7,    3,    3,    3,    3,    4,    5, IIP1,   22,
/* 0x4X */   IIP1, IIP1, IIP1,    2, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,    2, IIP1,    2,    2, IIP1,    2,
/* 0x5X */   IIP1, IIP1, IIP1,    2, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,    2, IIP1,    2,    2, IIP1,    2,
/* 0x6X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x7X */   IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1, IIP1,
/* 0x8X */      3,    3, IIP1,    4, IIP1, IIP1,    3, IIP1, IIP1, IIP1, IIP1,    3,    4,   25,   34,   28,
/* 0x9X */      4,    4, IIP1,    5, IIP1, IIP1,    4,    4, IIP1, IIP1, IIP1,    4,    5,   26,   35,   29,
/* 0xAX */      5,    5, IIP1,    6, IIP1, IIP1,    5,    5, IIP1, IIP1, IIP1,    5,    6,   27,   36,   30,
/* 0xBX */      5,    5, IIP1,    6, IIP1, IIP1,    5,    5, IIP1, IIP1, IIP1,    5,    6,   27,   36,   30,
/* 0xCX */      3,    3, IIP1, IIP1, IIP1, IIP1,    3, IIP1, IIP1, IIP1, IIP1,    3, IIP1, IIP1, IIP1, IIP1,
/* 0xDX */      4,    4, IIP1, IIP1, IIP1, IIP1,    4,    4, IIP1, IIP1, IIP1,    4, IIP1, IIP1, IIP1, IIP1,
/* 0xEX */      5,    5, IIP1, IIP1, IIP1, IIP1,    5,    5, IIP1, IIP1, IIP1,    5, IIP1, IIP1, IIP1, IIP1,
/* 0xFX */      5,    5, IIP1, IIP1, IIP1, IIP1,    5,    5, IIP1, IIP1, IIP1,    5, IIP1, IIP1, IIP1, IIP1
};

#ifndef BIG_SWITCH

static void (*hd6309_main[0x100])(void) = {
/*	        0xX0,   0xX1,     0xX2,    0xX3,    0xX4,    0xX5,    0xX6,    0xX7,
			0xX8,   0xX9,     0xXA,    0xXB,    0xXC,    0xXD,    0xXE,    0xXF   */

/* 0x0X */  neg_di,  oim_di,  aim_di,  com_di,  lsr_di,  eim_di,  ror_di,  asr_di,
            asl_di,  rol_di,  dec_di,  tim_di,  inc_di,  tst_di,  jmp_di,  clr_di,

/* 0x1X */  pref10,  pref11,  nop,     sync,    sexw,    IIError, lbra,    lbsr,
            IIError, daa,     orcc,    IIError, andcc,   sex,     exg,     tfr,

/* 0x2X */  bra,     brn,     bhi,     bls,     bcc,     bcs,     bne,     beq,
            bvc,     bvs,     bpl,     bmi,     bge,     blt,     bgt,     ble,

/* 0x3X */  leax,    leay,    leas,    leau,    pshs,    puls,    pshu,    pulu,
            IIError, rts,     abx,     rti,     cwai,    mul,     IIError, swi,

/* 0x4X */  nega,    IIError, IIError, coma,    lsra,    IIError, rora,    asra,
            asla,    rola,    deca,    IIError, inca,    tsta,    IIError, clra,

/* 0x5X */  negb,    IIError, IIError, comb,    lsrb,    IIError, rorb,    asrb,
            aslb,    rolb,    decb,    IIError, incb,    tstb,    IIError, clrb,

/* 0x6X */  neg_ix,  oim_ix,  aim_ix,  com_ix,  lsr_ix,  eim_ix,  ror_ix,  asr_ix,
            asl_ix,  rol_ix,  dec_ix,  tim_ix,  inc_ix,  tst_ix,  jmp_ix,  clr_ix,

/* 0x7X */  neg_ex,  oim_ex,  aim_ex,  com_ex,  lsr_ex,  eim_ex,  ror_ex,  asr_ex,
            asl_ex,  rol_ex,  dec_ex,  tim_ex,  inc_ex,  tst_ex,  jmp_ex,  clr_ex,

/* 0x8X */  suba_im, cmpa_im, sbca_im, subd_im, anda_im, bita_im, lda_im,  IIError,
            eora_im, adca_im, ora_im,  adda_im, cmpx_im, bsr,     ldx_im,  IIError,

/* 0x9X */  suba_di, cmpa_di, sbca_di, subd_di, anda_di, bita_di, lda_di,  sta_di,
            eora_di, adca_di, ora_di,  adda_di, cmpx_di, jsr_di,  ldx_di,  stx_di,

/* 0xAX */  suba_ix, cmpa_ix, sbca_ix, subd_ix, anda_ix, bita_ix, lda_ix,  sta_ix,
            eora_ix, adca_ix, ora_ix,  adda_ix, cmpx_ix, jsr_ix,  ldx_ix,  stx_ix,

/* 0xBX */  suba_ex, cmpa_ex, sbca_ex, subd_ex, anda_ex, bita_ex, lda_ex,  sta_ex,
            eora_ex, adca_ex, ora_ex,  adda_ex, cmpx_ex, jsr_ex,  ldx_ex,  stx_ex,

/* 0xCX */  subb_im, cmpb_im, sbcb_im, addd_im, andb_im, bitb_im, ldb_im,  IIError,
            eorb_im, adcb_im, orb_im,  addb_im, ldd_im,  ldq_im,  ldu_im,  IIError,

/* 0xDX */  subb_di, cmpb_di, sbcb_di, addd_di, andb_di, bitb_di, ldb_di,  stb_di,
            eorb_di, adcb_di, orb_di,  addb_di, ldd_di,  std_di,  ldu_di,  stu_di,

/* 0xEX */  subb_ix, cmpb_ix, sbcb_ix, addd_ix, andb_ix, bitb_ix, ldb_ix,  stb_ix,
            eorb_ix, adcb_ix, orb_ix,  addb_ix, ldd_ix,  std_ix,  ldu_ix,  stu_ix,

/* 0xFX */  subb_ex, cmpb_ex, sbcb_ex, addd_ex, andb_ex, bitb_ex, ldb_ex,  stb_ex,
            eorb_ex, adcb_ex, orb_ex,  addb_ex, ldd_ex,  std_ex,  ldu_ex,  stu_ex
};

static void (*hd6309_page01[0x100])(void) = {
/*	        0xX0,   0xX1,     0xX2,    0xX3,    0xX4,    0xX5,    0xX6,    0xX7,
			0xX8,   0xX9,     0xXA,    0xXB,    0xXC,    0xXD,    0xXE,    0xXF   */

/* 0x0X */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,

/* 0x1X */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,

/* 0x2X */  IIError, lbrn,    lbhi,    lbls,    lbcc,    lbcs,    lbne,    lbeq,
			lbvc,    lbvs,    lbpl,    lbmi,    lbge,    lblt,    lbgt,    lble,

/* 0x3X */  addr_r,  adcr,    subr,    sbcr,    andr,    orr,     eorr,    cmpr,
			pshsw,   pulsw,   pshuw,   puluw,   IIError, IIError, IIError, swi2,

/* 0x4X */  negd,    IIError, IIError, comd,    lsrd,    IIError, rord,    asrd,
			asld,    rold,    decd,    IIError, incd,    tstd,    IIError, clrd,

/* 0x5X */  IIError, IIError, IIError, comw,    lsrw,    IIError, rorw,    IIError,
			IIError, rolw,    decw,    IIError, incw,    tstw,    IIError, clrw,

/* 0x6X */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,

/* 0x7X */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,

/* 0x8X */  subw_im, cmpw_im, sbcd_im, cmpd_im, andd_im, bitd_im, ldw_im,  IIError,
			eord_im, adcd_im, ord_im,  addw_im, cmpy_im, IIError, ldy_im,  IIError,

/* 0x9X */  subw_di, cmpw_di, sbcd_di, cmpd_di, andd_di, bitd_di, ldw_di,  stw_di,
			eord_di, adcd_di, ord_di,  addw_di, cmpy_di, IIError, ldy_di,  sty_di,

/* 0xAX */  subw_ix, cmpw_ix, sbcd_ix, cmpd_ix, andd_ix, bitd_ix, ldw_ix,  stw_ix,
			eord_ix, adcd_ix, ord_ix,  addw_ix, cmpy_ix, IIError, ldy_ix,  sty_ix,

/* 0xBX */  subw_ex, cmpw_ex, sbcd_ex, cmpd_ex, andd_ex, bitd_ex, ldw_ex,  stw_ex,
			eord_ex, adcd_ex, ord_ex,  addw_ex, cmpy_ex, IIError, ldy_ex,  sty_ex,

/* 0xCX */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, IIError, IIError, lds_im,  IIError,

/* 0xDX */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, ldq_di,  stq_di,  lds_di,  sts_di,

/* 0xEX */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, ldq_ix,  stq_ix,  lds_ix,  sts_ix,

/* 0xFX */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, ldq_ex,  stq_ex,  lds_ex,  sts_ex
};
static void (*hd6309_page11[0x100])(void) = {
/*	        0xX0,   0xX1,     0xX2,    0xX3,    0xX4,    0xX5,    0xX6,    0xX7,
			0xX8,   0xX9,     0xXA,    0xXB,    0xXC,    0xXD,    0xXE,    0xXF   */

/* 0x0X */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,

/* 0x1X */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,

/* 0x2X */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,

/* 0x3X */  band,    biand,   bor,     bior,    beor,    bieor,   ldbt,    stbt,
			tfmpp,   tfmmm,   tfmpc,   tfmcp,   bitmd_im,ldmd_im, IIError, swi3,

/* 0x4X */  IIError, IIError, IIError, come,    IIError, IIError, IIError, IIError,
			IIError, IIError, dece,    IIError, ince,    tste,    IIError, clre,

/* 0x5X */  IIError, IIError, IIError, comf,    IIError, IIError, IIError, IIError,
			IIError, IIError, decf,    IIError, incf,    tstf,    IIError, clrf,

/* 0x6X */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,

/* 0x7X */  IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,
			IIError, IIError, IIError, IIError, IIError, IIError, IIError, IIError,

/* 0x8X */  sube_im, cmpe_im, IIError, cmpu_im, IIError, IIError, lde_im,  IIError,
			IIError, IIError, IIError, adde_im, cmps_im, divd_im, divq_im, muld_im,

/* 0x9X */  sube_di, cmpe_di, IIError, cmpu_di, IIError, IIError, lde_di,  ste_di,
			IIError, IIError, IIError, adde_di, cmps_di, divd_di, divq_di, muld_di,

/* 0xAX */  sube_ix, cmpe_ix, IIError, cmpu_ix, IIError, IIError, lde_ix,  ste_ix,
			IIError, IIError, IIError, adde_ix, cmps_ix, divd_ix, divq_ix, muld_ix,

/* 0xBX */  sube_ex, cmpe_ex, IIError, cmpu_ex, IIError, IIError, lde_ex,  ste_ex,
			IIError, IIError, IIError, adde_ex, cmps_ex, divd_ex, divq_ex, muld_ex,

/* 0xCX */  subf_im, cmpf_im, IIError, IIError, IIError, IIError, ldf_im,  IIError,
			IIError, IIError, IIError, addf_im, IIError, IIError, IIError, IIError,

/* 0xDX */  subf_di, cmpf_di, IIError, IIError, IIError, IIError, ldf_di,  stf_di,
			IIError, IIError, IIError, addf_di, IIError, IIError, IIError, IIError,

/* 0xEX */  subf_ix, cmpf_ix, IIError, IIError, IIError, IIError, ldf_ix,  stf_ix,
			IIError, IIError, IIError, addf_ix, IIError, IIError, IIError, IIError,

/* 0xFX */  subf_ex, cmpf_ex, IIError, IIError, IIError, IIError, ldf_ex,  stf_ex,
			IIError, IIError, IIError, addf_ex, IIError, IIError, IIError, IIError

};

#endif /* BIG_SWITCH */
