static INLINE void aba(void);
static INLINE void abx(void);
static INLINE void adca_di(void);
static INLINE void adca_ex(void);
static INLINE void adca_im(void);
static INLINE void adca_ix(void);
static INLINE void adcb_di(void);
static INLINE void adcb_ex(void);
static INLINE void adcb_im(void);
static INLINE void adcb_ix(void);
static INLINE void adcx_im(void);
static INLINE void adda_di(void);
static INLINE void adda_ex(void);
static INLINE void adda_im(void);
static INLINE void adda_ix(void);
static INLINE void addb_di(void);
static INLINE void addb_ex(void);
static INLINE void addb_im(void);
static INLINE void addb_ix(void);
static INLINE void addd_di(void);
static INLINE void addd_ex(void);
static INLINE void addx_ex(void);
static INLINE void addd_im(void);
static INLINE void addd_ix(void);
static INLINE void aim_di(void);
static INLINE void aim_ix(void);
static INLINE void anda_di(void);
static INLINE void anda_ex(void);
static INLINE void anda_im(void);
static INLINE void anda_ix(void);
static INLINE void andb_di(void);
static INLINE void andb_ex(void);
static INLINE void andb_im(void);
static INLINE void andb_ix(void);
static INLINE void asl_ex(void);
static INLINE void asl_ix(void);
static INLINE void asla(void);
static INLINE void aslb(void);
static INLINE void asld(void);
static INLINE void asr_ex(void);
static INLINE void asr_ix(void);
static INLINE void asra(void);
static INLINE void asrb(void);
static INLINE void bcc(void);
static INLINE void bcs(void);
static INLINE void beq(void);
static INLINE void bge(void);
static INLINE void bgt(void);
static INLINE void bhi(void);
static INLINE void bita_di(void);
static INLINE void bita_ex(void);
static INLINE void bita_im(void);
static INLINE void bita_ix(void);
static INLINE void bitb_di(void);
static INLINE void bitb_ex(void);
static INLINE void bitb_im(void);
static INLINE void bitb_ix(void);
static INLINE void ble(void);
static INLINE void bls(void);
static INLINE void blt(void);
static INLINE void bmi(void);
static INLINE void bne(void);
static INLINE void bpl(void);
static INLINE void bra(void);
static INLINE void brn(void);
static INLINE void bsr(void);
static INLINE void bvc(void);
static INLINE void bvs(void);
static INLINE void cba(void);
static INLINE void clc(void);
static INLINE void cli(void);
static INLINE void clr_ex(void);
static INLINE void clr_ix(void);
static INLINE void clra(void);
static INLINE void clrb(void);
static INLINE void clv(void);
static INLINE void cmpa_di(void);
static INLINE void cmpa_ex(void);
static INLINE void cmpa_im(void);
static INLINE void cmpa_ix(void);
static INLINE void cmpb_di(void);
static INLINE void cmpb_ex(void);
static INLINE void cmpb_im(void);
static INLINE void cmpb_ix(void);
static INLINE void cmpx_di(void);
static INLINE void cmpx_ex(void);
static INLINE void cmpx_im(void);
static INLINE void cmpx_ix(void);
static INLINE void com_ex(void);
static INLINE void com_ix(void);
static INLINE void coma(void);
static INLINE void comb(void);
static INLINE void daa(void);
static INLINE void dec_ex(void);
static INLINE void dec_ix(void);
static INLINE void deca(void);
static INLINE void decb(void);
static INLINE void des(void);
static INLINE void dex(void);
static INLINE void eim_di(void);
static INLINE void eim_ix(void);
static INLINE void eora_di(void);
static INLINE void eora_ex(void);
static INLINE void eora_im(void);
static INLINE void eora_ix(void);
static INLINE void eorb_di(void);
static INLINE void eorb_ex(void);
static INLINE void eorb_im(void);
static INLINE void eorb_ix(void);
/*INLINE void illegal(void);*/
static void illegal(void);
static INLINE void inc_ex(void);
static INLINE void inc_ix(void);
static INLINE void inca(void);
static INLINE void incb(void);
static INLINE void ins(void);
static INLINE void inx(void);
static INLINE void jmp_ex(void);
static INLINE void jmp_ix(void);
static INLINE void jsr_di(void);
static INLINE void jsr_ex(void);
static INLINE void jsr_ix(void);
static INLINE void lda_di(void);
static INLINE void lda_ex(void);
static INLINE void lda_im(void);
static INLINE void lda_ix(void);
static INLINE void ldb_di(void);
static INLINE void ldb_ex(void);
static INLINE void ldb_im(void);
static INLINE void ldb_ix(void);
static INLINE void ldd_di(void);
static INLINE void ldd_ex(void);
static INLINE void ldd_im(void);
static INLINE void ldd_ix(void);
static INLINE void lds_di(void);
static INLINE void lds_ex(void);
static INLINE void lds_im(void);
static INLINE void lds_ix(void);
static INLINE void ldx_di(void);
static INLINE void ldx_ex(void);
static INLINE void ldx_im(void);
static INLINE void ldx_ix(void);
static INLINE void lsr_ex(void);
static INLINE void lsr_ix(void);
static INLINE void lsra(void);
static INLINE void lsrb(void);
static INLINE void lsrd(void);
static INLINE void mul(void);
static INLINE void neg_ex(void);
static INLINE void neg_ix(void);
static INLINE void nega(void);
static INLINE void negb(void);
static INLINE void nop(void);
static INLINE void oim_di(void);
static INLINE void oim_ix(void);
static INLINE void ora_di(void);
static INLINE void ora_ex(void);
static INLINE void ora_im(void);
static INLINE void ora_ix(void);
static INLINE void orb_di(void);
static INLINE void orb_ex(void);
static INLINE void orb_im(void);
static INLINE void orb_ix(void);
static INLINE void psha(void);
static INLINE void pshb(void);
static INLINE void pshx(void);
static INLINE void pula(void);
static INLINE void pulb(void);
static INLINE void pulx(void);
static INLINE void rol_ex(void);
static INLINE void rol_ix(void);
static INLINE void rola(void);
static INLINE void rolb(void);
static INLINE void ror_ex(void);
static INLINE void ror_ix(void);
static INLINE void rora(void);
static INLINE void rorb(void);
static INLINE void rti(void);
static INLINE void rts(void);
static INLINE void sba(void);
static INLINE void sbca_di(void);
static INLINE void sbca_ex(void);
static INLINE void sbca_im(void);
static INLINE void sbca_ix(void);
static INLINE void sbcb_di(void);
static INLINE void sbcb_ex(void);
static INLINE void sbcb_im(void);
static INLINE void sbcb_ix(void);
static INLINE void sec(void);
static INLINE void sei(void);
static INLINE void sev(void);
#if (HAS_HD63701)
static INLINE void slp(void);
#endif
static INLINE void sta_di(void);
static INLINE void sta_ex(void);
static INLINE void sta_im(void);
static INLINE void sta_ix(void);
static INLINE void stb_di(void);
static INLINE void stb_ex(void);
static INLINE void stb_im(void);
static INLINE void stb_ix(void);
static INLINE void std_di(void);
static INLINE void std_ex(void);
static INLINE void std_im(void);
static INLINE void std_ix(void);
static INLINE void sts_di(void);
static INLINE void sts_ex(void);
static INLINE void sts_im(void);
static INLINE void sts_ix(void);
static INLINE void stx_di(void);
static INLINE void stx_ex(void);
static INLINE void stx_im(void);
static INLINE void stx_ix(void);
static INLINE void suba_di(void);
static INLINE void suba_ex(void);
static INLINE void suba_im(void);
static INLINE void suba_ix(void);
static INLINE void subb_di(void);
static INLINE void subb_ex(void);
static INLINE void subb_im(void);
static INLINE void subb_ix(void);
static INLINE void subd_di(void);
static INLINE void subd_ex(void);
static INLINE void subd_im(void);
static INLINE void subd_ix(void);
static INLINE void swi(void);
static INLINE void tab(void);
static INLINE void tap(void);
static INLINE void tba(void);
static INLINE void tim_di(void);
static INLINE void tim_ix(void);
static INLINE void tpa(void);
static INLINE void tst_ex(void);
static INLINE void tst_ix(void);
static INLINE void tsta(void);
static INLINE void tstb(void);
static INLINE void tsx(void);
static INLINE void txs(void);
static INLINE void undoc1(void);
static INLINE void undoc2(void);
static INLINE void wai(void);
static INLINE void xgdx(void);

static INLINE void cpx_di(void);
static INLINE void cpx_ex(void);
static INLINE void cpx_im(void);
static INLINE void cpx_ix(void);
/*INLINE void trap(void);*/
static void trap(void);

static void (*m6800_insn[0x100])(void) = {
illegal,nop,	illegal,illegal,illegal,illegal,tap,	tpa,
inx,	dex,	clv,	sev,	clc,	sec,	cli,	sei,
sba,	cba,	illegal,illegal,illegal,illegal,tab,	tba,
illegal,daa,	illegal,aba,	illegal,illegal,illegal,illegal,
bra,	brn,	bhi,	bls,	bcc,	bcs,	bne,	beq,
bvc,	bvs,	bpl,	bmi,	bge,	blt,	bgt,	ble,
tsx,	ins,	pula,	pulb,	des,	txs,	psha,	pshb,
illegal,rts,	illegal,rti,	illegal,illegal,wai,	swi,
nega,	illegal,illegal,coma,	lsra,	illegal,rora,	asra,
asla,	rola,	deca,	illegal,inca,	tsta,	illegal,clra,
negb,	illegal,illegal,comb,	lsrb,	illegal,rorb,	asrb,
aslb,	rolb,	decb,	illegal,incb,	tstb,	illegal,clrb,
neg_ix, illegal,illegal,com_ix, lsr_ix, illegal,ror_ix, asr_ix,
asl_ix, rol_ix, dec_ix, illegal,inc_ix, tst_ix, jmp_ix, clr_ix,
neg_ex, illegal,illegal,com_ex, lsr_ex, illegal,ror_ex, asr_ex,
asl_ex, rol_ex, dec_ex, illegal,inc_ex, tst_ex, jmp_ex, clr_ex,
suba_im,cmpa_im,sbca_im,illegal,anda_im,bita_im,lda_im, sta_im,
eora_im,adca_im,ora_im, adda_im,cmpx_im,bsr,	lds_im, sts_im,
suba_di,cmpa_di,sbca_di,illegal,anda_di,bita_di,lda_di, sta_di,
eora_di,adca_di,ora_di, adda_di,cmpx_di,jsr_di, lds_di, sts_di,
suba_ix,cmpa_ix,sbca_ix,illegal,anda_ix,bita_ix,lda_ix, sta_ix,
eora_ix,adca_ix,ora_ix, adda_ix,cmpx_ix,jsr_ix, lds_ix, sts_ix,
suba_ex,cmpa_ex,sbca_ex,illegal,anda_ex,bita_ex,lda_ex, sta_ex,
eora_ex,adca_ex,ora_ex, adda_ex,cmpx_ex,jsr_ex, lds_ex, sts_ex,
subb_im,cmpb_im,sbcb_im,illegal,andb_im,bitb_im,ldb_im, stb_im,
eorb_im,adcb_im,orb_im, addb_im,illegal,illegal,ldx_im, stx_im,
subb_di,cmpb_di,sbcb_di,illegal,andb_di,bitb_di,ldb_di, stb_di,
eorb_di,adcb_di,orb_di, addb_di,illegal,illegal,ldx_di, stx_di,
subb_ix,cmpb_ix,sbcb_ix,illegal,andb_ix,bitb_ix,ldb_ix, stb_ix,
eorb_ix,adcb_ix,orb_ix, addb_ix,illegal,illegal,ldx_ix, stx_ix,
subb_ex,cmpb_ex,sbcb_ex,illegal,andb_ex,bitb_ex,ldb_ex, stb_ex,
eorb_ex,adcb_ex,orb_ex, addb_ex,illegal,illegal,ldx_ex, stx_ex
};

#if (HAS_M6803)
static void (*m6803_insn[0x100])(void) = {
illegal,nop,	illegal,illegal,lsrd,	asld,	tap,	tpa,
inx,	dex,	clv,	sev,	clc,	sec,	cli,	sei,
sba,	cba,	illegal,illegal,illegal,illegal,tab,	tba,
illegal,daa,	illegal,aba,	illegal,illegal,illegal,illegal,
bra,	brn,	bhi,	bls,	bcc,	bcs,	bne,	beq,
bvc,	bvs,	bpl,	bmi,	bge,	blt,	bgt,	ble,
tsx,	ins,	pula,	pulb,	des,	txs,	psha,	pshb,
pulx,	rts,	abx,	rti,	pshx,	mul,	wai,	swi,
nega,	illegal,illegal,coma,	lsra,	illegal,rora,	asra,
asla,	rola,	deca,	illegal,inca,	tsta,	illegal,clra,
negb,	illegal,illegal,comb,	lsrb,	illegal,rorb,	asrb,
aslb,	rolb,	decb,	illegal,incb,	tstb,	illegal,clrb,
neg_ix, illegal,illegal,com_ix, lsr_ix, illegal,ror_ix, asr_ix,
asl_ix, rol_ix, dec_ix, illegal,inc_ix, tst_ix, jmp_ix, clr_ix,
neg_ex, illegal,illegal,com_ex, lsr_ex, illegal,ror_ex, asr_ex,
asl_ex, rol_ex, dec_ex, illegal,inc_ex, tst_ex, jmp_ex, clr_ex,
suba_im,cmpa_im,sbca_im,subd_im,anda_im,bita_im,lda_im, sta_im,
eora_im,adca_im,ora_im, adda_im,cpx_im ,bsr,	lds_im, sts_im,
suba_di,cmpa_di,sbca_di,subd_di,anda_di,bita_di,lda_di, sta_di,
eora_di,adca_di,ora_di, adda_di,cpx_di ,jsr_di, lds_di, sts_di,
suba_ix,cmpa_ix,sbca_ix,subd_ix,anda_ix,bita_ix,lda_ix, sta_ix,
eora_ix,adca_ix,ora_ix, adda_ix,cpx_ix ,jsr_ix, lds_ix, sts_ix,
suba_ex,cmpa_ex,sbca_ex,subd_ex,anda_ex,bita_ex,lda_ex, sta_ex,
eora_ex,adca_ex,ora_ex, adda_ex,cpx_ex ,jsr_ex, lds_ex, sts_ex,
subb_im,cmpb_im,sbcb_im,addd_im,andb_im,bitb_im,ldb_im, stb_im,
eorb_im,adcb_im,orb_im, addb_im,ldd_im, std_im, ldx_im, stx_im,
subb_di,cmpb_di,sbcb_di,addd_di,andb_di,bitb_di,ldb_di, stb_di,
eorb_di,adcb_di,orb_di, addb_di,ldd_di, std_di, ldx_di, stx_di,
subb_ix,cmpb_ix,sbcb_ix,addd_ix,andb_ix,bitb_ix,ldb_ix, stb_ix,
eorb_ix,adcb_ix,orb_ix, addb_ix,ldd_ix, std_ix, ldx_ix, stx_ix,
subb_ex,cmpb_ex,sbcb_ex,addd_ex,andb_ex,bitb_ex,ldb_ex, stb_ex,
eorb_ex,adcb_ex,orb_ex, addb_ex,ldd_ex, std_ex, ldx_ex, stx_ex
};
#endif

#if (HAS_HD63701)
static void (*hd63701_insn[0x100])(void) = {
trap	,nop,	trap	,trap	,lsrd,	asld,	tap,	tpa,
inx,	dex,	clv,	sev,	clc,	sec,	cli,	sei,
sba,	cba,	undoc1, undoc2, trap	,trap	,tab,	tba,
xgdx,	daa,	slp		,aba,	trap	,trap	,trap	,trap	,
bra,	brn,	bhi,	bls,	bcc,	bcs,	bne,	beq,
bvc,	bvs,	bpl,	bmi,	bge,	blt,	bgt,	ble,
tsx,	ins,	pula,	pulb,	des,	txs,	psha,	pshb,
pulx,	rts,	abx,	rti,	pshx,	mul,	wai,	swi,
nega,	trap	,trap	,coma,	lsra,	trap	,rora,	asra,
asla,	rola,	deca,	trap	,inca,	tsta,	trap	,clra,
negb,	trap	,trap	,comb,	lsrb,	trap	,rorb,	asrb,
aslb,	rolb,	decb,	trap	,incb,	tstb,	trap	,clrb,
neg_ix, aim_ix, oim_ix, com_ix, lsr_ix, eim_ix, ror_ix, asr_ix,
asl_ix, rol_ix, dec_ix, tim_ix, inc_ix, tst_ix, jmp_ix, clr_ix,
neg_ex, aim_di, oim_di, com_ex, lsr_ex, eim_di, ror_ex, asr_ex,
asl_ex, rol_ex, dec_ex, tim_di, inc_ex, tst_ex, jmp_ex, clr_ex,
suba_im,cmpa_im,sbca_im,subd_im,anda_im,bita_im,lda_im, sta_im,
eora_im,adca_im,ora_im, adda_im,cpx_im ,bsr,	lds_im, sts_im,
suba_di,cmpa_di,sbca_di,subd_di,anda_di,bita_di,lda_di, sta_di,
eora_di,adca_di,ora_di, adda_di,cpx_di ,jsr_di, lds_di, sts_di,
suba_ix,cmpa_ix,sbca_ix,subd_ix,anda_ix,bita_ix,lda_ix, sta_ix,
eora_ix,adca_ix,ora_ix, adda_ix,cpx_ix ,jsr_ix, lds_ix, sts_ix,
suba_ex,cmpa_ex,sbca_ex,subd_ex,anda_ex,bita_ex,lda_ex, sta_ex,
eora_ex,adca_ex,ora_ex, adda_ex,cpx_ex ,jsr_ex, lds_ex, sts_ex,
subb_im,cmpb_im,sbcb_im,addd_im,andb_im,bitb_im,ldb_im, stb_im,
eorb_im,adcb_im,orb_im, addb_im,ldd_im, std_im, ldx_im, stx_im,
subb_di,cmpb_di,sbcb_di,addd_di,andb_di,bitb_di,ldb_di, stb_di,
eorb_di,adcb_di,orb_di, addb_di,ldd_di, std_di, ldx_di, stx_di,
subb_ix,cmpb_ix,sbcb_ix,addd_ix,andb_ix,bitb_ix,ldb_ix, stb_ix,
eorb_ix,adcb_ix,orb_ix, addb_ix,ldd_ix, std_ix, ldx_ix, stx_ix,
subb_ex,cmpb_ex,sbcb_ex,addd_ex,andb_ex,bitb_ex,ldb_ex, stb_ex,
eorb_ex,adcb_ex,orb_ex, addb_ex,ldd_ex, std_ex, ldx_ex, stx_ex
};
#endif

#if (HAS_NSC8105)
static void (*nsc8105_insn[0x100])(void) = {
illegal,illegal,nop,	illegal,illegal,tap,	illegal,tpa,
inx,	clv,	dex,	sev,	clc,	cli,	sec,	sei,
sba,	illegal,cba,	illegal,illegal,tab,	illegal,tba,
illegal,illegal,daa,	aba,	illegal,illegal,illegal,illegal,
bra,	bhi,	brn,	bls,	bcc,	bne,	bcs,	beq,
bvc,	bpl,	bvs,	bmi,	bge,	bgt,	blt,	ble,
tsx,	pula,	ins,	pulb,	des,	psha,	txs,	pshb,
illegal,illegal,rts,	rti,	illegal,wai,	illegal,swi,
suba_im,sbca_im,cmpa_im,illegal,anda_im,lda_im, bita_im,sta_im,
eora_im,ora_im, adca_im,adda_im,cmpx_im,lds_im, bsr,	sts_im,
suba_di,sbca_di,cmpa_di,illegal,anda_di,lda_di, bita_di,sta_di,
eora_di,ora_di, adca_di,adda_di,cmpx_di,lds_di, jsr_di, sts_di,
suba_ix,sbca_ix,cmpa_ix,illegal,anda_ix,lda_ix, bita_ix,sta_ix,
eora_ix,ora_ix, adca_ix,adda_ix,cmpx_ix,lds_ix, jsr_ix, sts_ix,
suba_ex,sbca_ex,cmpa_ex,illegal,anda_ex,lda_ex, bita_ex,sta_ex,
eora_ex,ora_ex, adca_ex,adda_ex,cmpx_ex,lds_ex, jsr_ex, sts_ex,
nega,	illegal,illegal,coma,	lsra,	rora,	illegal,asra,
asla,	deca,	rola,	illegal,inca,	illegal,tsta,	clra,
negb,	illegal,illegal,comb,	lsrb,	rorb,	illegal,asrb,
aslb,	decb,	rolb,	illegal,incb,	illegal,tstb,	clrb,
neg_ix, illegal,illegal,com_ix, lsr_ix, ror_ix,	illegal,asr_ix,
asl_ix, dec_ix, rol_ix, illegal,inc_ix, jmp_ix, tst_ix, clr_ix,
neg_ex, illegal,illegal,com_ex, lsr_ex, ror_ex,	illegal,asr_ex,
asl_ex, dec_ex, rol_ex, illegal,inc_ex, jmp_ex, tst_ex, clr_ex,
subb_im,sbcb_im,cmpb_im,illegal,andb_im,ldb_im, bitb_im,stb_im,
eorb_im,orb_im, adcb_im,addb_im,illegal,ldx_im, illegal,stx_im,
subb_di,sbcb_di,cmpb_di,illegal,andb_di,ldb_di, bitb_di,stb_di,
eorb_di,orb_di, adcb_di,addb_di,illegal,ldx_di, illegal,stx_di,
subb_ix,sbcb_ix,cmpb_ix,illegal,andb_ix,ldb_ix, bitb_ix,stb_ix,
eorb_ix,orb_ix, adcb_ix,addb_ix,adcx_im,ldx_ix, illegal,stx_ix,
subb_ex,sbcb_ex,cmpb_ex,illegal,andb_ex,ldb_ex, bitb_ex,stb_ex,
eorb_ex,orb_ex, adcb_ex,addb_ex,addx_ex,ldx_ex, illegal,stx_ex
};
#endif
