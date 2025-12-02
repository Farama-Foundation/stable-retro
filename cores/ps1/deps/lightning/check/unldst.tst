#define GENTABLE		0
#define USEFUNC			0

#if __WORDSIZE == 32
#define GVAL			0xfedcba98
#else
#define GVAL			0xfedcba9876543210
#endif
#define FVAL			2.5
#define DVAL			7.5

#if USEFUNC
#  define LOAD(NAME, TYPE, R0, BASE, SIZE, RESULT)			\
	prepare								\
		pushargi BASE						\
		pushargi SIZE						\
	finishi NAME##TYPE						\
	retval %R0							\
	beqi NAME##TYPE##R0##BASE##SIZE %R0 RESULT			\
	calli @abort							\
NAME##TYPE##R0##BASE##SIZE:
#else
#  define LOAD(NAME, TYPE, R0, BASE, SIZE, RESULT)			\
	movi %R0 BASE							\
	NAME##r##TYPE %R0 %R0 SIZE					\
	beqi NAME##r##TYPE##R0##BASE##SIZE %R0 RESULT			\
	calli @abort							\
NAME##r##TYPE##R0##BASE##SIZE:						\
	NAME##i##TYPE %R0 BASE SIZE					\
	beqi NAME##i##TYPE##R0##BASE##SIZE %R0 RESULT			\
	calli @abort							\
NAME##i##TYPE##R0##BASE##SIZE:
#endif
#define LOAD1(NAME, TYPE, R0, R1, R2, V0, V1, V2, BASE, SIZE, RESULT)	\
	LOAD(NAME, TYPE, R0, BASE, SIZE, RESULT)			\
	LOAD(NAME, TYPE, R1, BASE, SIZE, RESULT)			\
	LOAD(NAME, TYPE, R2, BASE, SIZE, RESULT)			\
	LOAD(NAME, TYPE, V0, BASE, SIZE, RESULT)			\
	LOAD(NAME, TYPE, V1, BASE, SIZE, RESULT)			\
	LOAD(NAME, TYPE, V2, BASE, SIZE, RESULT)
#define  UNLD(BASE, SIZE, RESULT)					\
	LOAD1(unld, , r0, r1, r2, v0, v1, v2, BASE, SIZE, RESULT)
#define UNLDU(BASE, SIZE, RESULT)					\
	LOAD1(unld, _u, r0, r1, r2, v0, v1, v2, BASE, SIZE, RESULT)

#if USEFUNC
#  define STORE(R0, R1, R2, BASE, SIZE, RES0, RES1)			\
	movi %R0 str0							\
	movi %R1 0							\
	str %R0 %R1							\
	stxi $(__WORDSIZE >> 3) %R0 %R1					\
	movi %R0 GVAL							\
	prepare								\
		pushargi BASE						\
		pushargr %R0						\
		pushargi SIZE						\
	finishi unst							\
	movi %R0 str0							\
	ldr %R1 %R0							\
	ldxi %R2 %R0 $(__WORDSIZE >> 3)					\
	bnei unst##R0##R1##R2##BASE##SIZE##fail %R1 RES0		\
	beqi unst##R0##R1##R2##BASE##SIZE %R2 RES1			\
unst##R0##R1##R2##BASE##SIZE##fail:					\
	calli @abort							\
unst##R0##R1##R2##BASE##SIZE:
#else
#  define STORE(R0, R1, R2, BASE, SIZE, RES0, RES1)			\
	movi %R0 str0							\
	movi %R1 0							\
	str %R0 %R1							\
	stxi $(__WORDSIZE >> 3) %R0 %R1					\
	movi %R0 GVAL							\
	movi %R1 BASE							\
	unstr %R1 %R0 SIZE						\
	movi %R0 str0							\
	ldr %R1 %R0							\
	ldxi %R2 %R0 $(__WORDSIZE >> 3)					\
	bnei unst##r##R0##R1##R2##BASE##SIZE##fail %R1 RES0		\
	beqi unst##r##R0##R1##R2##BASE##SIZE %R2 RES1			\
unst##r##R0##R1##R2##BASE##SIZE##fail:					\
	calli @abort							\
unst##r##R0##R1##R2##BASE##SIZE:					\
	movi %R0 str0							\
	movi %R1 0							\
	str %R0 %R1							\
	stxi $(__WORDSIZE >> 3) %R0 %R1					\
	movi %R0 GVAL							\
	unsti BASE %R0 SIZE						\
	movi %R0 str0							\
	ldr %R1 %R0							\
	ldxi %R2 %R0 $(__WORDSIZE >> 3)					\
	bnei unst##i##R0##R1##R2##BASE##SIZE##fail %R1 RES0		\
	beqi unst##i##R0##R1##R2##BASE##SIZE %R2 RES1			\
unst##i##R0##R1##R2##BASE##SIZE##fail:					\
	calli @abort							\
unst##i##R0##R1##R2##BASE##SIZE:
#endif
#define STORE1(R0, R1, R2, V0, V1, V2, BASE, SIZE, RES0, RES1)		\
	STORE(R0, R1, R2, BASE, SIZE, RES0, RES1)			\
	STORE(R1, R2, V0, BASE, SIZE, RES0, RES1)			\
	STORE(R2, V0, V1, BASE, SIZE, RES0, RES1)			\
	STORE(V0, V1, V2, BASE, SIZE, RES0, RES1)			\
	STORE(V1, V2, R0, BASE, SIZE, RES0, RES1)			\
	STORE(V2, R0, R1, BASE, SIZE, RES0, RES1)
#define  UNST(BASE, SIZE, RES0, RES1)					\
	STORE1(r0, r1, r2, v0, v1, v2, BASE, SIZE, RES0, RES1)

#if USEFUNC
#  define F_LDST(F0, BASE, VAL)						\
	movi %r2 str0							\
	movi %r1 0							\
	str %r2 %r1							\
	stxi $(__WORDSIZE >> 3) %r2 %r1					\
	movi_f %F0 VAL							\
	sti_f cvt %F0							\
	ldi_i %r0 cvt							\
	prepare								\
		pushargi BASE						\
		pushargr %r0						\
	finishi st4							\
	prepare								\
		pushargi BASE						\
	finishi ld4							\
	retval %r0							\
	sti_i cvt %r0							\
	movi_f %F0 -VAL							\
	ldi_f %F0 cvt							\
	beqi_f f##F0##BASE %F0 VAL					\
	calli @abort							\
f##F0##BASE:
#else
#  define F_LDST(F0, BASE, VAL)						\
	movi %v0 BASE							\
	movi_f %F0 VAL							\
	unstr_x %v0 %F0 4						\
	movi_f %F0 0							\
	unldr_x %F0 %v0 4						\
	beqi_f fr##F0##BASE %F0 VAL					\
	calli @abort							\
fr##F0##BASE:								\
	movi_f %F0 VAL							\
	unsti_x BASE %F0 4						\
	movi_f %F0 0							\
	unldi_x %F0 BASE 4						\
	beqi_f fi##F0##BASE %F0 VAL					\
	calli @abort							\
fi##F0##BASE:
#endif
#define FLDST1(F0, F1, F2, F3, F4, F5, BASE, VAL)			\
	F_LDST(F0, BASE, VAL)						\
	F_LDST(F1, BASE, VAL)						\
	F_LDST(F2, BASE, VAL)						\
	F_LDST(F3, BASE, VAL)						\
	F_LDST(F4, BASE, VAL)						\
	F_LDST(F5, BASE, VAL)
#define FLDST(BASE, VAL)						\
	FLDST1(f0, f1, f2, f3, f4, f5, BASE, VAL)

#if USEFUNC
#  if __WORDSIZE == 32
#    if __BYTE_ORDER == __LITTLE_ENDIAN
#      define D_LDST(F0, BASE, VAL)					\
	movi %r2 BASE							\
	movi %r1 0							\
	str %r2 %r1							\
	stxi 4 %r2 %r1							\
	movi_d %F0 VAL							\
	sti_d cvt %F0							\
	movi %r0 cvt							\
	ldr %r0 %r0							\
	prepare								\
		pushargi BASE						\
		pushargr %r0						\
	finishi st4							\
	prepare								\
		pushargi BASE						\
	finishi ld4							\
	retval %v2							\
	movi %r0 cvt							\
	addi %r0 %r0 4							\
	ldr %r0 %r0							\
	movi %v0 BASE							\
	addi %v0 %v0 4							\
	prepare								\
		pushargr %v0						\
		pushargr %r0						\
	finishi st4							\
	prepare								\
		pushargr %v0						\
	finishi ld4							\
	retval %r0							\
	movi %r1 cvt							\
	str %r1 %v2							\
	stxi 4 %r1 %r0							\
	ldi_d %F0 cvt							\
	beqi_d d##F0##BASE %F0 VAL					\
	calli @abort							\
d##F0##BASE:
#    else
#      define D_LDST(F0, BASE, VAL)					\
	movi %r2 BASE							\
	movi %r1 0							\
	str %r2 %r1							\
	stxi 4 %r2 %r1							\
	movi_d %F0 VAL							\
	sti_d cvt %F0							\
	movi %r0 cvt							\
	addi %r0 %r0 4							\
	ldr %r0 %r0							\
	prepare								\
		pushargi BASE						\
		pushargr %r0						\
	finishi st4							\
	prepare								\
		pushargi BASE						\
	finishi ld4							\
	retval %v2							\
	movi %r0 cvt							\
	ldr %r0 %r0							\
	movi %v0 BASE							\
	addi %v0 %v0 4							\
	prepare								\
		pushargr %v0						\
		pushargr %r0						\
	finishi st4							\
	prepare								\
		pushargr %v0						\
	finishi ld4							\
	retval %r0							\
	movi %r1 cvt							\
	str %r1 %r0							\
	stxi 4 %r1 %v2							\
	ldi_d %F0 cvt							\
	beqi_d d##F0##BASE %F0 VAL					\
	calli @abort							\
d##F0##BASE:
#    endif
#  else
#    define D_LDST(F0, BASE, VAL)					\
	movi %r2 str0							\
	movi %r1 0							\
	stxi 8 %r2 %r1							\
	movi_d %F0 DVAL							\
	sti_d cvt %F0							\
	ldi %r0 cvt							\
	prepare								\
		pushargi BASE						\
		pushargr %r0						\
	finishi st8							\
	prepare								\
		pushargi BASE						\
	finishi ld8							\
	retval %r0							\
	sti cvt %r0							\
	ldi_d %F0 cvt							\
	beqi_d d##F0##BASE %F0 VAL					\
	calli @abort							\
d##F0##BASE:
#  endif
#else
#  define D_LDST(F0, BASE, VAL)						\
	movi %v0 BASE							\
	movi_d %F0 VAL							\
	unstr_x %v0 %F0 8						\
	movi_d %F0 0							\
	unldr_x %F0 %v0 8						\
	beqi_d dr##F0##BASE %F0 VAL					\
	calli @abort							\
dr##F0##BASE:								\
	movi_d %F0 VAL							\
	unsti_x BASE %F0 8						\
	movi_d %F0 0							\
	unldi_x %F0 BASE 8						\
	beqi_d di##F0##BASE %F0 VAL					\
	calli @abort							\
di##F0##BASE:
#endif

#define DLDST1(F0, F1, F2, F3, F4, F5, BASE, VAL)			\
	D_LDST(F0, BASE, VAL)						\
	D_LDST(F1, BASE, VAL)						\
	D_LDST(F2, BASE, VAL)						\
	D_LDST(F3, BASE, VAL)						\
	D_LDST(F4, BASE, VAL)						\
	D_LDST(F5, BASE, VAL)
#define DLDST(BASE, VAL)						\
	DLDST1(f0, f1, f2, f3, f4, f5, BASE, VAL)

.data	4096
ok:
.c	"ok"
sUNLD:
.c	" UNLD"
sUNLDU:
.c	"UNLDU"
fmt_ldf:
.c	"\tFLDST(str%d, %.1f)\n"
fmt_ldd:
.c	"\tDLDST(str%d, %.1f)\n"
.align	8
cvt:
.size	8
str0:
.c	0x00
str1:
.c	0x00
str2:
.c	0x00
str3:
.c	0x00
str4:
.c	0x00
str5:
.c	0x00
str6:
.c	0x00
str7:
.c	0x00
str8:
.c	0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
#if __WORDSIZE == 32
buf0:
.c	0x80
buf1:
.c	0x81
buf2:
.c	0x82
buf3:
.c	0x83 0x84 0x85 0x86 0x87
fmt:
.c	"0x%08x\n"
fmt_ld:
.c	"\t%s(buf%d, %d, 0x%08x)\n"
fmt_st:
.c	"\t UNST(str%d, %d, 0x%08x, 0x%08x)\n"
#else
buf0:
.c	0x80
buf1:
.c	0x81
buf2:
.c	0x82
buf3:
.c	0x83
buf4:
.c	0x84
buf5:
.c	0x85
buf6:
.c	0x86
buf7:
.c	0x87 0x88 0x89 0x8a 0x8b 0x8c 0x8d 0x8e 0x8f
fmt:
.c	"0x%016lx\n"
fmt_ld:
.c	"\t%s(buf%d, %d, 0x%016lx)\n"
fmt_st:
.c	"\t UNST(str%d, %d, 0x%016lx, 0x%016lx)\n"
#endif

.code
	jmpi main

ld2:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -2
	bner ld2_un2 %r1 %r2
	ldr_s %r0 %r1
	jmpi ld2_al
ld2_un2:
#if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_c %r2 %r1 1
	lshi %r2 %r2 8
#else
	ldr_c %r0 %r1
	lshi %r0 %r0 8
	ldxi_uc %r2 %r1 1
#endif
	orr %r0 %r0 %r2
ld2_al:
	retr %r0
	epilog

ld2u:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -2
	bner ld2u_un2 %r1 %r2
	ldr_us %r0 %r1
	jmpi ld2u_al
ld2u_un2:
	ldr_uc %r0 %r1
	ldxi_uc %r2 %r1 1
#if __BYTE_ORDER == __LITTLE_ENDIAN
	lshi %r2 %r2 8
#else
	lshi %r0 %r0 8
#endif
	orr %r0 %r0 %r2
ld2u_al:
	retr %r0
	epilog

ld3:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -2
	bner ld3_un2 %r1 %r2
#if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_us %r0 %r1
	ldxi_c %r2 %r1 2
	lshi %r2 %r2 16
#else
	ldr_s %r0 %r1
	lshi %r0 %r0 8
	ldxi_uc %r2 %r1 2
#endif
	jmpi ld3_or
ld3_un2:
#if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_s %r2 %r1 1
	lshi %r2 %r2 8
#else
	ldr_c %r0 %r1
	lshi %r0 %r0 16
	ldxi_us %r2 %r1 1
#endif
ld3_or:
	orr %r0 %r0 %r2
	retr %r0
	epilog

ld3u:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -2
	bner ld3u_un2 %r1 %r2
#if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_us %r0 %r1
	ldxi_uc %r2 %r1 2
	lshi %r2 %r2 16
#else
	ldr_us %r0 %r1
	lshi %r0 %r0 8
	ldxi_uc %r2 %r1 2
#endif
	jmpi ld3u_or
ld3u_un2:
#if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 8
#else
	ldr_uc %r0 %r1
	lshi %r0 %r0 16
	ldxi_us %r2 %r1 1
#endif
ld3u_or:
	orr %r0 %r0 %r2
	retr %r0
	epilog

ld4:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -4
	bner ld4_un4 %r1 %r2
	ldr_i %r0 %r1
	jmpi ld4_al
ld4_un4:
	andi %r2 %r1 -2
	bner ld4_un2 %r1 %r2
#if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_us %r0 %r1
	ldxi_s %r2 %r1 2
	lshi %r2 %r2 16
#else
	ldr_s %r0 %r1
	lshi %r0 %r0 16
	ldxi_us %r2 %r1 2
#endif
	jmpi ld4_or
	// assume address is mapped in a multiple of 4 as it will read
	// one out of bounds byte to reduce number of used instructions
ld4_un2:
	andi %r2 %r1 3
	bnei ld4_un3 %r2 3
#if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_i %r2 %r1 1
	lshi %r2 %r2 8
#  if __WORDSIZE == 64
	extr_i %r2 %r2
#  endif
#else
	ldr_c %r0 %r1
	lshi %r0 %r0 24
#  if __WORDSIZE == 32
	ldxi %r2 %r1 1
#  else
	ldxi_ui %r2 %r1 1
#  endif
	rshi_u %r2 %r2 8
#endif
	jmpi ld4_or
ld4_un3:
#if __BYTE_ORDER == __LITTLE_ENDIAN
#  if __WORDSIZE == 32
	ldxi %r0 %r1 -1
#  else
	ldxi_ui %r0 %r1 -1
#  endif
	rshi_u %r0 %r0 8
	ldxi_c %r2 %r1 3
	lshi %r2 %r2 24
#else
	ldxi_i %r0 %r1 -1
	lshi %r0 %r0 8
#  if __WORDSIZE == 64
	extr_i %r0 %r0
#  endif
	ldxi_uc %r2 %r1 3
#endif
ld4_or:
	orr %r0 %r0 %r2
ld4_al:
	retr %r0
	epilog

#if __WORDSIZE == 64
ld4u:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -4
	bner ld4u_un4 %r1 %r2
	ldr_ui %r0 %r1
	jmpi ld4u_al
ld4u_un4:
	andi %r2 %r1 -2
	bner ld4u_un2 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_us %r0 %r1
	ldxi_us %r2 %r1 2
	lshi %r2 %r2 16
#  else
	ldr_us %r0 %r1
	lshi %r0 %r0 16
	ldxi_us %r2 %r1 2
#  endif
	jmpi ld4u_or
	// assume address is mapped in a multiple of 4 as it will read
	// one out of bounds byte to reduce number of used instructions
ld4u_un2:
	andi %r2 %r1 3
	bnei ld4u_un3 %r2 3
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_ui %r2 %r1 1
	lshi %r2 %r2 8
	extr_ui %r2 %r2
#  else
	ldr_uc %r0 %r1
	lshi %r0 %r0 24
	ldxi_ui %r2 %r1 1
	rshi_u %r2 %r2 8
#  endif
	jmpi ld4u_or
ld4u_un3:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldxi_ui %r0 %r1 -1
	rshi %r0 %r0 8
	ldxi_uc %r2 %r1 3
	lshi %r2 %r2 24
#  else
	ldxi_ui %r0 %r1 -1
	lshi %r0 %r0 8
	extr_ui %r0 %r0
	ldxi_uc %r2 %r1 3
#  endif
ld4u_or:
	orr %r0 %r0 %r2
ld4u_al:
	retr %r0
	epilog

ld5:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -4
	bner ld5_un4 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_ui %r0 %r1
	ldxi_c %r2 %r1 4
	lshi %r2 %r2 32
#  else
	ldr_i %r0 %r1
	lshi %r0 %r0 8
	ldxi_uc %r2 %r1 4
#  endif
	jmpi ld5_or
ld5_un4:
	andi %r2 %r1 -2
	bner ld5_un2 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_us %r0 %r1
	ldxi_us %r2 %r1 2
	lshi %r2 %r2 16
	orr %r0 %r0 %r2
	ldxi_c %r2 %r1 4
	lshi %r2 %r2 32
#  else
	ldr_s %r0 %r1
	lshi %r0 %r0 24
	ldxi_us %r2 %r1 2
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 4
#  endif
	jmpi ld5_or
ld5_un2:
	andi %r2 %r1 3
	bnei ld5_un3 %r2 3
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_i %r2 %r1 1
	lshi %r2 %r2 8
#  else
	ldr_c %r0 %r1
	lshi %r0 %r0 32
	ldxi_ui %r2 %r1 1
#  endif
	jmpi ld5_or
ld5_un3:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_s %r2 %r1 3
	lshi %r2 %r2 24
#  else
	ldr_c %r0 %r1
	lshi %r0 %r0 32
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 16
	orr %r0 %r0 %r2
	ldxi_us %r2 %r1 3
#  endif
ld5_or:
	orr %r0 %r0 %r2
	retr %r0
	epilog

ld5u:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -4
	bner ld5u_un4 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_ui %r0 %r1
	ldxi_uc %r2 %r1 4
	lshi %r2 %r2 32
#  else
	ldr_ui %r0 %r1
	lshi %r0 %r0 8
	ldxi_uc %r2 %r1 4
#  endif
	jmpi ld5u_or
ld5u_un4:
	andi %r2 %r1 -2
	bner ld5u_un2 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_us %r0 %r1
	ldxi_us %r2 %r1 2
	lshi %r2 %r2 16
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 4
	lshi %r2 %r2 32
#  else
	ldr_us %r0 %r1
	lshi %r0 %r0 24
	ldxi_us %r2 %r1 2
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 4
#  endif
	jmpi ld5u_or
ld5u_un2:
	andi %r2 %r1 3
	bnei ld5u_un3 %r2 3
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_ui %r2 %r1 1
	lshi %r2 %r2 8
#  else
	ldr_uc %r0 %r1
	lshi %r0 %r0 32
	ldxi_ui %r2 %r1 1
#  endif
	jmpi ld5u_or
ld5u_un3:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_us %r2 %r1 3
	lshi %r2 %r2 24
#  else
	ldr_uc %r0 %r1
	lshi %r0 %r0 32
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 16
	orr %r0 %r0 %r2
	ldxi_us %r2 %r1 3
#  endif
ld5u_or:
	orr %r0 %r0 %r2
	retr %r0
	epilog

ld6:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -4
	bner ld6_un4 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_ui %r0 %r1
	ldxi_s %r2 %r1 4
	lshi %r2 %r2 32
#  else
	ldr_i %r0 %r1
	lshi %r0 %r0 16
	ldxi_us %r2 %r1 4
#  endif
	jmpi ld6_or
ld6_un4:
	andi %r2 %r1 -2
	bner ld6_un2 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_us %r0 %r1
	ldxi_i %r2 %r1 2
	lshi %r2 %r2 16
#  else
	ldr_s %r0 %r1
	lshi %r0 %r0 32
	ldxi_ui %r2 %r1 2
#  endif
	jmpi ld6_or
ld6_un2:
	andi %r2 %r1 3
	bnei ld6_un3 %r2 3
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_ui %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_c %r2 %r1 5
	lshi %r2 %r2 40
#  else
	ldr_c %r0 %r1
	lshi %r0 %r0 40
	ldxi_ui %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 5
#  endif
	jmpi ld6_or
ld6_un3:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_us %r2 %r1 3
	lshi %r2 %r2 24
	orr %r0 %r0 %r2
	ldxi_c %r2 %r1 5
	lshi %r2 %r2 40
#  else
	ldr_c %r0 %r1
	lshi %r0 %r0 40
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 24
	orr %r0 %r0 %r2
	ldxi_us %r2 %r1 3
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 5
#  endif
ld6_or:
	orr %r0 %r0 %r2
	retr %r0
	epilog

ld6u:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -4
	bner ld6u_un4 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_ui %r0 %r1
	ldxi_us %r2 %r1 4
	lshi %r2 %r2 32
#  else
	ldr_ui %r0 %r1
	lshi %r0 %r0 16
	ldxi_us %r2 %r1 4
#  endif
	jmpi ld6u_or
ld6u_un4:
	andi %r2 %r1 -2
	bner ld6u_un2 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_us %r0 %r1
	ldxi_ui %r2 %r1 2
	lshi %r2 %r2 16
#  else
	ldr_us %r0 %r1
	lshi %r0 %r0 32
	ldxi_ui %r2 %r1 2
#  endif
	jmpi ld6u_or
ld6u_un2:
	andi %r2 %r1 3
	bnei ld6u_un3 %r2 3
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_ui %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 5
	lshi %r2 %r2 40
#  else
	ldr_uc %r0 %r1
	lshi %r0 %r0 40
	ldxi_ui %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 5
#  endif
	jmpi ld6u_or
ld6u_un3:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_us %r2 %r1 3
	lshi %r2 %r2 24
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 5
	lshi %r2 %r2 40
#  else
	ldr_uc %r0 %r1
	lshi %r0 %r0 40
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 24
	orr %r0 %r0 %r2
	ldxi_us %r2 %r1 3
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 5
#  endif
ld6u_or:
	orr %r0 %r0 %r2
	retr %r0
	epilog

ld7:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -4
	bner ld7_un4 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_ui %r0 %r1
	ldxi_i %r2 %r1 4
	lshi %r2 %r2 40
	rshi %r2 %r2 8
#  else
	ldr_i %r0 %r1
	lshi %r0 %r0 24
	ldxi_ui %r2 %r1 4
	rshi %r2 %r2 8
#  endif
	jmpi ld7_or
ld7_un4:
	andi %r2 %r1 -2
	bner ld7_un2 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_us %r0 %r1
	ldxi_ui %r2 %r1 2
	lshi %r2 %r2 16
	orr %r0 %r0 %r2
	ldxi_c %r2 %r1 6
	lshi %r2 %r2 48
#  else
	ldr_s %r0 %r1
	lshi %r0 %r0 40
	ldxi_ui %r2 %r1 2
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 6
#  endif
	jmpi ld7_or
ld7_un2:
	andi %r2 %r1 3
	bnei ld7_un3 %r2 3
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_ui %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_s %r2 %r1 5
	lshi %r2 %r2 40
#else
	ldr_c %r0 %r1
	lshi %r0 %r0 48
	ldxi_ui %r2 %r1 1
	lshi %r2 %r2 16
	orr %r0 %r0 %r2
	ldxi_us %r2 %r1 5
#  endif
	jmpi ld7_or
ld7_un3:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_i %r2 %r1 3
	lshi %r2 %r2 24
#  else
	ldr_c %r0 %r1
	lshi %r0 %r0 48
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 32
	orr %r0 %r0 %r2
	ldxi_ui %r2 %r1 3
#  endif
ld7_or:
	orr %r0 %r0 %r2
	retr %r0
	epilog

ld7u:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -4
	bner ld7u_un4 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_ui %r0 %r1
	ldxi_ui %r2 %r1 4
	lshi %r2 %r2 40
	rshi_u %r2 %r2 8
#  else
	ldr_ui %r0 %r1
	lshi %r0 %r0 24
	ldxi_ui %r2 %r1 4
	rshi_u %r2 %r2 8
#  endif
	jmpi ld7u_or
ld7u_un4:
	andi %r2 %r1 -2
	bner ld7u_un2 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_us %r0 %r1
	ldxi_ui %r2 %r1 2
	lshi %r2 %r2 16
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 6
	lshi %r2 %r2 48
#  else
	ldr_us %r0 %r1
	lshi %r0 %r0 40
	ldxi_ui %r2 %r1 2
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 6
#  endif
	jmpi ld7u_or
ld7u_un2:
	andi %r2 %r1 3
	bnei ld7u_un3 %r2 3
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_ui %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_us %r2 %r1 5
	lshi %r2 %r2 40
#else
	ldr_uc %r0 %r1
	lshi %r0 %r0 48
	ldxi_ui %r2 %r1 1
	lshi %r2 %r2 16
	orr %r0 %r0 %r2
	ldxi_us %r2 %r1 5
#  endif
	jmpi ld7u_or
ld7u_un3:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_ui %r2 %r1 3
	lshi %r2 %r2 24
#  else
	ldr_uc %r0 %r1
	lshi %r0 %r0 48
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 32
	orr %r0 %r0 %r2
	ldxi_ui %r2 %r1 3
#  endif
ld7u_or:
	orr %r0 %r0 %r2
	retr %r0
	epilog

ld8:
	prolog
	arg $addr
	getarg %r1 $addr
	andi %r2 %r1 -8
	bner ld8_un8 %r1 %r2
	ldr_l %r0 %r1
	jmpi ld8_al
ld8_un8:
	andi %r2 %r1 -4
	bner ld8_un4 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_ui %r0 %r1
	ldxi_i %r2 %r1 4
	lshi %r2 %r2 32
#  else
	ldr_i %r0 %r1
	ldxi_ui %r2 %r1 4
	lshi %r0 %r0 32
#  endif
	jmpi ld8_or
ld8_un4:
	andi %r2 %r1 -2
	bner ld8_un2 %r1 %r2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_us %r0 %r1
	ldxi_ui %r2 %r1 2
	lshi %r2 %r2 16
	orr %r0 %r0 %r2
	ldxi_s %r2 %r1 6
	lshi %r2 %r2 48
#  else
	ldr_s %r0 %r1
	lshi %r0 %r0 48
	ldxi_ui %r2 %r1 2
	lshi %r2 %r2 16
	orr %r0 %r0 %r2
	ldxi_us %r2 %r1 6
#  endif
	jmpi ld8_or
ld8_un2:
	andi %r2 %r1 7
	bnei ld8_un7 %r2 7
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_l %r2 %r1 1
	lshi %r2 %r2 8
#  else
	ldr_c %r0 %r1
	ldxi_l %r2 %r1 1
	rshi_u %r2 %r2 8
	lshi %r0 %r0 56
#  endif
	jmpi ld8_or
ld8_un7:
	bnei ld8_un6 %r2 6
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_us %r0 %r1
	ldxi_l %r2 %r1 2
	lshi %r2 %r2 16
#  else
	ldr_s %r0 %r1
	lshi %r0 %r0 48
	ldxi_l %r2 %r1 2
	rshi_u %r2 %r2 16
#  endif
	jmpi ld8_or
ld8_un6:
	bnei ld8_un5 %r2 5
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldxi_ui %r0 %r1 -1
	rshi_u %r0 %r0 8
	ldxi_ui %r2 %r1 3
	lshi %r2 %r2 24
	orr %r0 %r0 %r2
	ldxi_c %r2 %r1 7
	lshi %r2 %r2 56
#  else
	ldxi_i %r0 %r1 -1
	lshi %r0 %r0 40
	ldxi_ui %r2 %r1 3
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 7
#  endif
	jmpi ld8_or
ld8_un5:
	bnei ld8_un3 %r2 3
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_ui %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_i %r2 %r1 5
	lshi %r2 %r2 40
#  else
	ldr_c %r0 %r1
	lshi %r0 %r0 56
	ldxi_ui %r2 %r1 1
	lshi %r2 %r2 24
	orr %r0 %r0 %r2
	ldxi_ui %r2 %r1 5
	rshi_u %r2 %r2 8
#  endif
	jmpi ld8_or
ld8_un3:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	ldr_uc %r0 %r1
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_ui %r2 %r1 3
	lshi %r2 %r2 24
	orr %r0 %r0 %r2
	ldxi_c %r2 %r1 7
	lshi %r2 %r2 56
#  else
	ldr_c %r0 %r1
	lshi %r0 %r0 56
	ldxi_us %r2 %r1 1
	lshi %r2 %r2 40
	orr %r0 %r0 %r2
	ldxi_ui %r2 %r1 3
	lshi %r2 %r2 8
	orr %r0 %r0 %r2
	ldxi_uc %r2 %r1 7
#  endif
ld8_or:
	orr %r0 %r0 %r2
ld8_al:
	retr %r0
	epilog
#endif

st2:
	prolog
	arg $addr
	arg $value
	getarg %r1 $addr
	getarg %r0 $value
	andi %r2 %r1 -2
	bner st2_un2 %r2 %r1
	str_s %r1 %r0
	jmpi st2_al
st2_un2:
#if __BYTE_ORDER == __LITTLE_ENDIAN
	str_c %r1 %r0
	rshi_u %r0 %r0 8
	stxi_c 1 %r1 %r0
#else
	stxi_c 1 %r1 %r0
	rshi_u %r0 %r0 8
	str_c %r1 %r0
#endif
st2_al:
	ret
	epilog

st3:
	prolog
	arg $addr
	arg $value
	getarg %r1 $addr
	getarg %r0 $value
	andi %r2 %r1 -2
	bner st3_un2 %r2 %r1
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_s %r1 %r0
	rshi %r0 %r0 16
	stxi_c 2 %r1 %r0
#  else
	stxi_c 2 %r1 %r0
	rshi %r0 %r0 8
	str_s %r1 %r0
#  endif
	jmpi st3_al
st3_un2:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_c %r1 %r0
	rshi %r0 %r0 8
	stxi_s 1 %r1 %r0
#  else
	stxi_s 1 %r1 %r0
	rshi %r0 %r0 16
	str_c %r1 %r0
#  endif
st3_al:
	ret
	epilog

st4:
	prolog
	arg $addr
	arg $value
	getarg %r1 $addr
	getarg %r0 $value
	andi %r2 %r1 -4
	bner st4_un4 %r2 %r1
	str_i %r1 %r0
	jmpi st4_al
st4_un4:
	andi %r2 %r1 -2
	bner st4_un2 %r2 %r1
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_s %r1 %r0
	rshi %r0 %r0 16
	stxi_s 2 %r1 %r0
#  else
	stxi_s 2 %r1 %r0
	rshi %r0 %r0 16
	str_s %r1 %r0
#  endif
	jmpi st4_al
st4_un2:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_c %r1 %r0
	rshi %r0 %r0 8
	stxi_s 1 %r1 %r0
	rshi %r0 %r0 16
	stxi_c 3 %r1 %r0
#  else
	stxi_c 3 %r1 %r0
	rshi %r0 %r0 8
	stxi_s 1 %r1 %r0
	rshi %r0 %r0 16
	str_c %r1 %r0
#  endif
st4_al:
	ret
	epilog

#if __WORDSIZE == 64
st5:
	prolog
	arg $addr
	arg $value
	getarg %r1 $addr
	getarg %r0 $value
	andi %r2 %r1 3
	bnei st5_un3 %r2 3
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_c %r1 %r0
	rshi %r0 %r0 8
	stxi_i 1 %r1 %r0
#  else
	stxi_i 1 %r1 %r0
	rshi %r0 %r0 32
	str_c %r1 %r0	
#  endif
	jmpi st5_al
st5_un3:
	bnei st5_un2 %r2 2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_s %r1 %r0
	rshi %r0 %r0 16
	stxi_s 2 %r1 %r0
	rshi %r0 %r0 16
	stxi_c 4 %r1 %r0
#  else
	stxi_c 4 %r1 %r0
	rshi %r0 %r0 8
	stxi_s 2 %r1 %r0
	rshi %r0 %r0 16
	str_s %r1 %r0
#  endif
	jmpi st5_al
st5_un2:
	bnei st5_un1 %r2 1
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_c %r1 %r0
	rshi %r0 %r0 8
	stxi_s 1 %r1 %r0
	rshi %r0 %r0 16
	stxi_s 3 %r1 %r0
#  else
	stxi_s 3 %r1 %r0
	rshi %r0 %r0 16
	stxi_s 1 %r1 %r0
	rshi %r0 %r0 16
	str_c %r1 %r0
#  endif
	jmpi st5_al
st5_un1:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_i %r1 %r0
	rshi %r0 %r0 32
	stxi_c 4 %r1 %r0
#  else
	stxi_c 4 %r1 %r0
	rshi %r0 %r0 8
	str_i %r1 %r0
#  endif
st5_al:
	ret
	epilog

st6:
	prolog
	arg $addr
	arg $value
	getarg %r1 $addr
	getarg %r0 $value
	andi %r2 %r1 3
	bnei st6_un3 %r2 3
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_c %r1 %r0
	rshi %r0 %r0 8
	stxi_i 1 %r1 %r0
	rshi %r0 %r0 32
	stxi_c 5 %r1 %r0
#  else
	stxi_c 5 %r1 %r0
	rshi %r0 %r0 8
	stxi_i 1 %r1 %r0
	rshi %r0 %r0 32
	str_c %r1 %r0	
#  endif
	jmpi st6_al
st6_un3:
	bnei st6_un2 %r2 2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_s %r1 %r0
	rshi %r0 %r0 16
	stxi_s 2 %r1 %r0
	rshi %r0 %r0 16
	stxi_s 4 %r1 %r0
#  else
	stxi_s 4 %r1 %r0
	rshi %r0 %r0 16
	stxi_s 2 %r1 %r0
	rshi %r0 %r0 16
	str_s %r1 %r0
#  endif
	jmpi st6_al
st6_un2:
	bnei st6_un1 %r2 1
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_c %r1 %r0
	rshi %r0 %r0 8
	stxi_s 1 %r1 %r0
	rshi %r0 %r0 16
	stxi_s 3 %r1 %r0
	rshi %r0 %r0 16
	stxi_c 5 %r1 %r0
#  else
	stxi_c 5 %r1 %r0
	rshi %r0 %r0 8
	stxi_s 3 %r1 %r0
	rshi %r0 %r0 16
	stxi_s 1 %r1 %r0
	rshi %r0 %r0 16
	str_c %r1 %r0
#  endif
	jmpi st6_al
st6_un1:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_i %r1 %r0
	rshi %r0 %r0 32
	stxi_s 4 %r1 %r0
#  else
	stxi_s 4 %r1 %r0
	rshi %r0 %r0 16
	str_i %r1 %r0
#  endif
st6_al:
	ret
	epilog

st7:
	prolog
	arg $addr
	arg $value
	getarg %r1 $addr
	getarg %r0 $value
	andi %r2 %r1 3
	bnei st7_un3 %r2 3
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_c %r1 %r0
	rshi %r0 %r0 8
	stxi_i 1 %r1 %r0
	rshi %r0 %r0 32
	stxi_s 5 %r1 %r0
#  else
	stxi_s 5 %r1 %r0
	rshi %r0 %r0 16
	stxi_i 1 %r1 %r0
	rshi %r0 %r0 32
	str_c %r1 %r0	
#  endif
	jmpi st7_al
st7_un3:
	bnei st7_un2 %r2 2
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_s %r1 %r0
	rshi %r0 %r0 16
	stxi_i 2 %r1 %r0
	rshi %r0 %r0 32
	stxi_c 6 %r1 %r0
#  else
	stxi_c 6 %r1 %r0
	rshi %r0 %r0 8
	stxi_i 2 %r1 %r0
	rshi %r0 %r0 32
	str_s %r1 %r0
#  endif
	jmpi st7_al
st7_un2:
	bnei st7_un1 %r2 1
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_c %r1 %r0
	rshi %r0 %r0 8
	stxi_s 1 %r1 %r0
	rshi %r0 %r0 16
	stxi_i 3 %r1 %r0
#  else
	stxi_i 3 %r1 %r0
	rshi %r0 %r0 32
	stxi_s 1 %r1 %r0
	rshi %r0 %r0 16
	str_c %r1 %r0
#  endif
	jmpi st7_al
st7_un1:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_i %r1 %r0
	rshi %r0 %r0 32
	stxi_s 4 %r1 %r0
	rshi %r0 %r0 16
	stxi_c 6 %r1 %r0
#  else
	stxi_c 6 %r1 %r0
	rshi %r0 %r0 8
	stxi_s 4 %r1 %r0
	rshi %r0 %r0 16
	str_i %r1 %r0
#  endif
st7_al:
	ret
	epilog

st8:
	prolog
	arg $addr
	arg $value
	getarg %r1 $addr
	getarg %r0 $value
	andi %r2 %r1 -8
	bner st8_un8 %r2 %r1
	str_l %r1 %r0
	jmpi st8_al
st8_un8:
	andi %r2 %r1 -4
	bner st8_un4 %r2 %r1
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_i %r1 %r0
	rshi %r0 %r0 32
	stxi_i 4 %r1 %r0
#  else
	stxi_i 4 %r1 %r0
	rshi %r0 %r0 32
	str_i %r1 %r0
#  endif
	jmpi st8_al
st8_un4:
	andi %r2 %r1 -2
	bner st8_un2 %r2 %r1
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_s %r1 %r0
	rshi %r0 %r0 16
	stxi_i 2 %r1 %r0
	rshi %r0 %r0 32
	stxi_s 6 %r1 %r0
#  else
	stxi_s 6 %r1 %r0
	rshi %r0 %r0 16
	stxi_i 2 %r1 %r0
	rshi %r0 %r0 32
	str_s %r1 %r0
#  endif
	jmpi st8_al
st8_un2:
	andi %r2 %r1 3
	bnei st8_un3 %r2 3
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_c %r1 %r0
	rshi %r0 %r0 8
	stxi_i 1 %r1 %r0
	rshi %r0 %r0 32
	stxi_s 5 %r1 %r0
	rshi %r0 %r0 16
	stxi_c 7 %r1 %r0
#  else
	stxi_c 7 %r1 %r0
	rshi %r0 %r0 8
	stxi_s 5 %r1 %r0
	rshi %r0 %r0 16
	stxi_i 1 %r1 %r0
	rshi %r0 %r0 32
	str_c %r1 %r0
#  endif
	jmpi st8_al
st8_un3:
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	str_c %r1 %r0
	rshi %r0 %r0 8
	stxi_s 1 %r1 %r0
	rshi %r0 %r0 16
	stxi_i 3 %r1 %r0
	rshi %r0 %r0 32
	stxi_c 7 %r1 %r0
#  else
	stxi_c 7 %r1 %r0
	rshi %r0 %r0 8
	stxi_i 3 %r1 %r0
	rshi %r0 %r0 32
	stxi_s 1 %r1 %r0
	rshi %r0 %r0 16
	str_c %r1 %r0
#  endif
st8_al:
	ret
	epilog
#endif

unld:
	prolog
	arg $base
	arg $size
	getarg %v0 $base
	getarg %v1 $size
	bnei unld2 %v1 1
	ldr_c %r0 %v0
	jmpi unld_done
unld2:
	bnei unld3 %v1 2
	prepare
		pushargr %v0
	finishi ld2
	retval %r0
	jmpi unld_done
unld3:
	bnei unld4 %v1 3
	prepare
		pushargr %v0
	finishi ld3
	retval %r0
	jmpi unld_done
unld4:
#if __WORDSIZE == 64
	bnei unld5 %v1 4
#else
	bnei unld_fail %v1 4
#endif
	prepare
		pushargr %v0
	finishi ld4
	retval %r0
	jmpi unld_done
#if __WORDSIZE == 64
unld5:
	bnei unld6 %v1 5
	prepare
		pushargr %v0
	finishi ld5
	retval %r0
	jmpi unld_done
unld6:
	bnei unld7 %v1 6
	prepare
		pushargr %v0
	finishi ld6
	retval %r0
	jmpi unld_done
unld7:
	bnei unld8 %v1 7
	prepare
		pushargr %v0
	finishi ld7
	retval %r0
	jmpi unld_done
unld8:
	bnei unld_fail %v1 8
	prepare
		pushargr %v0
	finishi ld8
	retval %r0
#endif
unld_done:
	retr %r0
unld_fail:
	calli @abort
	epilog

unld_u:
	prolog
	arg $base
	arg $size
	getarg %v0 $base
	getarg %v1 $size
	bnei unld_u2 %v1 1
	ldr_uc %r0 %v0
	jmpi unld_done
unld_u2:
	bnei unld_u3 %v1 2
	prepare
		pushargr %v0
	finishi ld2u
	retval %r0
	jmpi unld_u_done
unld_u3:
	bnei unld_u4 %v1 3
	prepare
		pushargr %v0
	finishi ld3u
	retval %r0
	jmpi unld_u_done
unld_u4:
#if __WORDSIZE == 64
	bnei unld_u5 %v1 4
#else
	bnei unld_fail %v1 4
#endif
	prepare
		pushargr %v0
#if __WORDSIZE == 32
	finishi ld4
#else
	finishi ld4u
#endif
	retval %r0
	jmpi unld_u_done
#if __WORDSIZE == 64
unld_u5:
	bnei unld_u6 %v1 5
	prepare
		pushargr %v0
	finishi ld5u
	retval %r0
	jmpi unld_u_done
unld_u6:
	bnei unld_u7 %v1 6
	prepare
		pushargr %v0
	finishi ld6u
	retval %r0
	jmpi unld_u_done
unld_u7:
	bnei unld_u8 %v1 7
	prepare
		pushargr %v0
	finishi ld7u
	retval %r0
	jmpi unld_u_done
unld_u8:
	bnei unld_u_fail %v1 8
	prepare
		pushargr %v0
	finishi ld8
	retval %r0
#endif
unld_u_done:
	retr %r0
unld_u_fail:
	calli @abort
	epilog

unst:
	prolog
	arg $base
	arg $data
	arg $size
	getarg %v0 $base
	getarg %v2 $data
	getarg %v1 $size
	bnei unst2 %v1 1
	str_c %v0 %v2
	jmpi unst_done
unst2:
	bnei unst3 %v1 2
	prepare
		pushargr %v0
		pushargr %v2
	finishi st2
	jmpi unst_done
unst3:
	bnei unst4 %v1 3
	prepare
		pushargr %v0
		pushargr %v2
	finishi st3
	jmpi unst_done
unst4:
#if __WORDSIZE == 64
	bnei unst5 %v1 4
#else
	bnei unst_fail %v1 4
#endif
	prepare
		pushargr %v0
		pushargr %v2
	finishi st4
#if __WORDSIZE == 64
	jmpi unst_done
unst5:
	bnei unst6 %v1 5
	prepare
		pushargr %v0
		pushargr %v2
	finishi st5
	jmpi unst_done
unst6:
	bnei unst7 %v1 6
	prepare
		pushargr %v0
		pushargr %v2
	finishi st6
	jmpi unst_done
unst7:
	bnei unst8 %v1 7
	prepare
		pushargr %v0
		pushargr %v2
	finishi st7
	jmpi unst_done
unst8:
	bnei unst_fail %v1 8
	prepare
		pushargr %v0
		pushargr %v2
	finishi st8
#endif
unst_done:
	ret
unst_fail:
	calli @abort
	epilog

main:
	prolog
#if GENTABLE
	movi %v0 buf0
out_loop_ld:
	movi %v1 GVAL
	movi %v2 1
loop_ld:
	prepare
		pushargr %v0
		pushargr %v2
	finishi unld
	retval %r0
	subi %r2 %v0 buf0
	prepare
		pushargi fmt_ld
		ellipsis
		pushargi sUNLD
		pushargr %r2
		pushargr %v2
		pushargr %r0
	finishi @printf
	addi %v2 %v2 1
	blei loop_ld %v2 $(__WORDSIZE >> 3)
	addi %v0 %v0 1
	subi %r2 %v0 buf0
	blti out_loop_ld %r2 $(__WORDSIZE >> 3)

	movi %v0 buf0
out_loop_ldu:
	movi %v2 1
loop_ldu:
	prepare
		pushargr %v0
		pushargr %v2
	finishi unld_u
	retval %r0
	subi %r2 %v0 buf0
	prepare
		pushargi fmt_ld
		ellipsis
		pushargi sUNLDU
		pushargr %r2
		pushargr %v2
		pushargr %r0
	finishi @printf
	addi %v2 %v2 1
	blei loop_ldu %v2 $(__WORDSIZE >> 3)
	addi %v0 %v0 1
	subi %r2 %v0 buf0
	blti out_loop_ldu %r2 $(__WORDSIZE >> 3)

	movi %v0 str0
out_loop_st:
	movi %v2 1
loop_st:
	movi %r0 str0
	movi %r1 0
	str %r0 %r1
	stxi $(__WORDSIZE >> 3) %r0 %r1
	prepare
		pushargr %v0
		pushargr %v1
		pushargr %v2
	finishi unst
	movi %r2 str0
	ldr %r0 %r2
	ldxi %r1 %r2 $(__WORDSIZE >> 3)
	subi %r2 %v0 str0
	prepare
		pushargi fmt_st
		ellipsis
		pushargr %r2
		pushargr %v2
		pushargr %r0
		pushargr %r1
	finishi @printf
	addi %v2 %v2 1
	blei loop_st %v2 $(__WORDSIZE >> 3)
	addi %v0 %v0 1
	subi %r2 %v0 str0
	blti out_loop_st %r2 $(__WORDSIZE >> 3)

	/* Do complex operations to also ensure sample fallbacks are correct */
	movi %v0 str0
	movi %v1 0
loop_f:
	movi %r2 str0
	movi %r1 0
	str %r2 %r1
	stxi $(__WORDSIZE >> 3) %r2 %r1
	movi_f %f0 FVAL
	sti_f cvt %f0
	ldi_i %r0 cvt
	prepare
		pushargr %v0
		pushargr %r0
	finishi st4
	prepare
		pushargr %v0
	finishi ld4
	retval %r0
	sti_i cvt %r0
	ldi_f %f0 cvt
	extr_f_d %f0 %f0
	prepare
		pushargi fmt_ldf
		ellipsis
		pushargr %v1
		pushargr_d %f0
	finishi @printf
	addi %v0 %v0 1
	addi %v1 %v1 1
	blei loop_f %v1 4

	movi %v0 str0
	movi %v1 0
loop_d:
	movi %r2 str0
	movi %r1 0
	str %r2 %r1
	stxi $(__WORDSIZE >> 3) %r2 %r1
	movi_d %f0 DVAL
	sti_d cvt %f0
#  if __WORDSIZE == 32
	movi %r0 cvt
#    if __BYTE_ORDER == __BIG_ENDIAN
	addi %r0 %r0 4
#    endif
	ldr %r0 %r0
	prepare
		pushargr %v0
		pushargr %r0
	finishi st4
	prepare
		pushargr %v0
	finishi ld4
	retval %v2
	movi %r0 cvt
#    if __BYTE_ORDER == __LITTLE_ENDIAN
	addi %r0 %r0 4
#    endif
	ldr %r0 %r0
	prepare
		pushargr %v0
		pushargr %r0
	finishi st4
	prepare
		pushargr %v0
	finishi ld4
	retval %r0
	movi %r1 cvt
#    if __BYTE_ORDER == __LITTLE_ENDIAN
	str %r1 %v1
	stxi 4 %r1 %r0
#    else
	str %r1 %r0
	stxi 4 %r1 %v1
#    endif
#  else
	ldi %r0 cvt
	prepare
		pushargr %v0
		pushargr %r0
	finishi st8
	prepare
		pushargr %v0
	finishi ld8
	retval %r0
	sti cvt %r0
#  endif
	ldi_d %f0 cvt
	prepare
		pushargi fmt_ldd
		ellipsis
		pushargr %v1
		pushargr_d %f0
	finishi @printf
	addi %v0 %v0 1
	addi %v1 %v1 1
	blei loop_d %v1 8
#else

#  if __WORDSIZE == 32
#    if __BYTE_ORDER == __LITTLE_ENDIAN
	 UNLD(buf0, 1, 0xffffff80)
	 UNLD(buf0, 2, 0xffff8180)
	 UNLD(buf0, 3, 0xff828180)
	 UNLD(buf0, 4, 0x83828180)
	 UNLD(buf1, 1, 0xffffff81)
	 UNLD(buf1, 2, 0xffff8281)
	 UNLD(buf1, 3, 0xff838281)
	 UNLD(buf1, 4, 0x84838281)
	 UNLD(buf2, 1, 0xffffff82)
	 UNLD(buf2, 2, 0xffff8382)
	 UNLD(buf2, 3, 0xff848382)
	 UNLD(buf2, 4, 0x85848382)
	 UNLD(buf3, 1, 0xffffff83)
	 UNLD(buf3, 2, 0xffff8483)
	 UNLD(buf3, 3, 0xff858483)
	 UNLD(buf3, 4, 0x86858483)
	UNLDU(buf0, 1, 0x00000080)
	UNLDU(buf0, 2, 0x00008180)
	UNLDU(buf0, 3, 0x00828180)
	UNLDU(buf0, 4, 0x83828180)
	UNLDU(buf1, 1, 0x00000081)
	UNLDU(buf1, 2, 0x00008281)
	UNLDU(buf1, 3, 0x00838281)
	UNLDU(buf1, 4, 0x84838281)
	UNLDU(buf2, 1, 0x00000082)
	UNLDU(buf2, 2, 0x00008382)
	UNLDU(buf2, 3, 0x00848382)
	UNLDU(buf2, 4, 0x85848382)
	UNLDU(buf3, 1, 0x00000083)
	UNLDU(buf3, 2, 0x00008483)
	UNLDU(buf3, 3, 0x00858483)
	UNLDU(buf3, 4, 0x86858483)
	 UNST(str0, 1, 0x00000098, 0x00000000)
	 UNST(str0, 2, 0x0000ba98, 0x00000000)
	 UNST(str0, 3, 0x00dcba98, 0x00000000)
	 UNST(str0, 4, 0xfedcba98, 0x00000000)
	 UNST(str1, 1, 0x00009800, 0x00000000)
	 UNST(str1, 2, 0x00ba9800, 0x00000000)
	 UNST(str1, 3, 0xdcba9800, 0x00000000)
	 UNST(str1, 4, 0xdcba9800, 0x000000fe)
	 UNST(str2, 1, 0x00980000, 0x00000000)
	 UNST(str2, 2, 0xba980000, 0x00000000)
	 UNST(str2, 3, 0xba980000, 0x000000dc)
	 UNST(str2, 4, 0xba980000, 0x0000fedc)
	 UNST(str3, 1, 0x98000000, 0x00000000)
	 UNST(str3, 2, 0x98000000, 0x000000ba)
	 UNST(str3, 3, 0x98000000, 0x0000dcba)
	 UNST(str3, 4, 0x98000000, 0x00fedcba)
#    else
	 UNLD(buf0, 1, 0xffffff80)
	 UNLD(buf0, 2, 0xffff8081)
	 UNLD(buf0, 3, 0xff808182)
	 UNLD(buf0, 4, 0x80818283)
	 UNLD(buf1, 1, 0xffffff81)
	 UNLD(buf1, 2, 0xffff8182)
	 UNLD(buf1, 3, 0xff818283)
	 UNLD(buf1, 4, 0x81828384)
	 UNLD(buf2, 1, 0xffffff82)
	 UNLD(buf2, 2, 0xffff8283)
	 UNLD(buf2, 3, 0xff828384)
	 UNLD(buf2, 4, 0x82838485)
	 UNLD(buf3, 1, 0xffffff83)
	 UNLD(buf3, 2, 0xffff8384)
	 UNLD(buf3, 3, 0xff838485)
	 UNLD(buf3, 4, 0x83848586)
	UNLDU(buf0, 1, 0x00000080)
	UNLDU(buf0, 2, 0x00008081)
	UNLDU(buf0, 3, 0x00808182)
	UNLDU(buf0, 4, 0x80818283)
	UNLDU(buf1, 1, 0x00000081)
	UNLDU(buf1, 2, 0x00008182)
	UNLDU(buf1, 3, 0x00818283)
	UNLDU(buf1, 4, 0x81828384)
	UNLDU(buf2, 1, 0x00000082)
	UNLDU(buf2, 2, 0x00008283)
	UNLDU(buf2, 3, 0x00828384)
	UNLDU(buf2, 4, 0x82838485)
	UNLDU(buf3, 1, 0x00000083)
	UNLDU(buf3, 2, 0x00008384)
	UNLDU(buf3, 3, 0x00838485)
	UNLDU(buf3, 4, 0x83848586)
	 UNST(str0, 1, 0x98000000, 0x00000000)
	 UNST(str0, 2, 0xba980000, 0x00000000)
	 UNST(str0, 3, 0xdcba9800, 0x00000000)
	 UNST(str0, 4, 0xfedcba98, 0x00000000)
	 UNST(str1, 1, 0x00980000, 0x00000000)
	 UNST(str1, 2, 0x00ba9800, 0x00000000)
	 UNST(str1, 3, 0x00dcba98, 0x00000000)
	 UNST(str1, 4, 0x00fedcba, 0x98000000)
	 UNST(str2, 1, 0x00009800, 0x00000000)
	 UNST(str2, 2, 0x0000ba98, 0x00000000)
	 UNST(str2, 3, 0x0000dcba, 0x98000000)
	 UNST(str2, 4, 0x0000fedc, 0xba980000)
	 UNST(str3, 1, 0x00000098, 0x00000000)
	 UNST(str3, 2, 0x000000ba, 0x98000000)
	 UNST(str3, 3, 0x000000dc, 0xba980000)
	 UNST(str3, 4, 0x000000fe, 0xdcba9800)
#    endif

#  else
#    if __BYTE_ORDER == __LITTLE_ENDIAN
	 UNLD(buf0, 1, 0xffffffffffffff80)
	 UNLD(buf0, 2, 0xffffffffffff8180)
	 UNLD(buf0, 3, 0xffffffffff828180)
	 UNLD(buf0, 4, 0xffffffff83828180)
	 UNLD(buf0, 5, 0xffffff8483828180)
	 UNLD(buf0, 6, 0xffff858483828180)
	 UNLD(buf0, 7, 0xff86858483828180)
	 UNLD(buf0, 8, 0x8786858483828180)
	 UNLD(buf1, 1, 0xffffffffffffff81)
	 UNLD(buf1, 2, 0xffffffffffff8281)
	 UNLD(buf1, 3, 0xffffffffff838281)
	 UNLD(buf1, 4, 0xffffffff84838281)
	 UNLD(buf1, 5, 0xffffff8584838281)
	 UNLD(buf1, 6, 0xffff868584838281)
	 UNLD(buf1, 7, 0xff87868584838281)
	 UNLD(buf1, 8, 0x8887868584838281)
	 UNLD(buf2, 1, 0xffffffffffffff82)
	 UNLD(buf2, 2, 0xffffffffffff8382)
	 UNLD(buf2, 3, 0xffffffffff848382)
	 UNLD(buf2, 4, 0xffffffff85848382)
	 UNLD(buf2, 5, 0xffffff8685848382)
	 UNLD(buf2, 6, 0xffff878685848382)
	 UNLD(buf2, 7, 0xff88878685848382)
	 UNLD(buf2, 8, 0x8988878685848382)
	 UNLD(buf3, 1, 0xffffffffffffff83)
	 UNLD(buf3, 2, 0xffffffffffff8483)
	 UNLD(buf3, 3, 0xffffffffff858483)
	 UNLD(buf3, 4, 0xffffffff86858483)
	 UNLD(buf3, 5, 0xffffff8786858483)
	 UNLD(buf3, 6, 0xffff888786858483)
	 UNLD(buf3, 7, 0xff89888786858483)
	 UNLD(buf3, 8, 0x8a89888786858483)
	 UNLD(buf4, 1, 0xffffffffffffff84)
	 UNLD(buf4, 2, 0xffffffffffff8584)
	 UNLD(buf4, 3, 0xffffffffff868584)
	 UNLD(buf4, 4, 0xffffffff87868584)
	 UNLD(buf4, 5, 0xffffff8887868584)
	 UNLD(buf4, 6, 0xffff898887868584)
	 UNLD(buf4, 7, 0xff8a898887868584)
	 UNLD(buf4, 8, 0x8b8a898887868584)
	 UNLD(buf5, 1, 0xffffffffffffff85)
	 UNLD(buf5, 2, 0xffffffffffff8685)
	 UNLD(buf5, 3, 0xffffffffff878685)
	 UNLD(buf5, 4, 0xffffffff88878685)
	 UNLD(buf5, 5, 0xffffff8988878685)
	 UNLD(buf5, 6, 0xffff8a8988878685)
	 UNLD(buf5, 7, 0xff8b8a8988878685)
	 UNLD(buf5, 8, 0x8c8b8a8988878685)
	 UNLD(buf6, 1, 0xffffffffffffff86)
	 UNLD(buf6, 2, 0xffffffffffff8786)
	 UNLD(buf6, 3, 0xffffffffff888786)
	 UNLD(buf6, 4, 0xffffffff89888786)
	 UNLD(buf6, 5, 0xffffff8a89888786)
	 UNLD(buf6, 6, 0xffff8b8a89888786)
	 UNLD(buf6, 7, 0xff8c8b8a89888786)
	 UNLD(buf6, 8, 0x8d8c8b8a89888786)
	 UNLD(buf7, 1, 0xffffffffffffff87)
	 UNLD(buf7, 2, 0xffffffffffff8887)
	 UNLD(buf7, 3, 0xffffffffff898887)
	 UNLD(buf7, 4, 0xffffffff8a898887)
	 UNLD(buf7, 5, 0xffffff8b8a898887)
	 UNLD(buf7, 6, 0xffff8c8b8a898887)
	 UNLD(buf7, 7, 0xff8d8c8b8a898887)
	 UNLD(buf7, 8, 0x8e8d8c8b8a898887)
	UNLDU(buf0, 1, 0x0000000000000080)
	UNLDU(buf0, 2, 0x0000000000008180)
	UNLDU(buf0, 3, 0x0000000000828180)
	UNLDU(buf0, 4, 0x0000000083828180)
	UNLDU(buf0, 5, 0x0000008483828180)
	UNLDU(buf0, 6, 0x0000858483828180)
	UNLDU(buf0, 7, 0x0086858483828180)
	UNLDU(buf0, 8, 0x8786858483828180)
	UNLDU(buf1, 1, 0x0000000000000081)
	UNLDU(buf1, 2, 0x0000000000008281)
	UNLDU(buf1, 3, 0x0000000000838281)
	UNLDU(buf1, 4, 0x0000000084838281)
	UNLDU(buf1, 5, 0x0000008584838281)
	UNLDU(buf1, 6, 0x0000868584838281)
	UNLDU(buf1, 7, 0x0087868584838281)
	UNLDU(buf1, 8, 0x8887868584838281)
	UNLDU(buf2, 1, 0x0000000000000082)
	UNLDU(buf2, 2, 0x0000000000008382)
	UNLDU(buf2, 3, 0x0000000000848382)
	UNLDU(buf2, 4, 0x0000000085848382)
	UNLDU(buf2, 5, 0x0000008685848382)
	UNLDU(buf2, 6, 0x0000878685848382)
	UNLDU(buf2, 7, 0x0088878685848382)
	UNLDU(buf2, 8, 0x8988878685848382)
	UNLDU(buf3, 1, 0x0000000000000083)
	UNLDU(buf3, 2, 0x0000000000008483)
	UNLDU(buf3, 3, 0x0000000000858483)
	UNLDU(buf3, 4, 0x0000000086858483)
	UNLDU(buf3, 5, 0x0000008786858483)
	UNLDU(buf3, 6, 0x0000888786858483)
	UNLDU(buf3, 7, 0x0089888786858483)
	UNLDU(buf3, 8, 0x8a89888786858483)
	UNLDU(buf4, 1, 0x0000000000000084)
	UNLDU(buf4, 2, 0x0000000000008584)
	UNLDU(buf4, 3, 0x0000000000868584)
	UNLDU(buf4, 4, 0x0000000087868584)
	UNLDU(buf4, 5, 0x0000008887868584)
	UNLDU(buf4, 6, 0x0000898887868584)
	UNLDU(buf4, 7, 0x008a898887868584)
	UNLDU(buf4, 8, 0x8b8a898887868584)
	UNLDU(buf5, 1, 0x0000000000000085)
	UNLDU(buf5, 2, 0x0000000000008685)
	UNLDU(buf5, 3, 0x0000000000878685)
	UNLDU(buf5, 4, 0x0000000088878685)
	UNLDU(buf5, 5, 0x0000008988878685)
	UNLDU(buf5, 6, 0x00008a8988878685)
	UNLDU(buf5, 7, 0x008b8a8988878685)
	UNLDU(buf5, 8, 0x8c8b8a8988878685)
	UNLDU(buf6, 1, 0x0000000000000086)
	UNLDU(buf6, 2, 0x0000000000008786)
	UNLDU(buf6, 3, 0x0000000000888786)
	UNLDU(buf6, 4, 0x0000000089888786)
	UNLDU(buf6, 5, 0x0000008a89888786)
	UNLDU(buf6, 6, 0x00008b8a89888786)
	UNLDU(buf6, 7, 0x008c8b8a89888786)
	UNLDU(buf6, 8, 0x8d8c8b8a89888786)
	UNLDU(buf7, 1, 0x0000000000000087)
	UNLDU(buf7, 2, 0x0000000000008887)
	UNLDU(buf7, 3, 0x0000000000898887)
	UNLDU(buf7, 4, 0x000000008a898887)
	UNLDU(buf7, 5, 0x0000008b8a898887)
	UNLDU(buf7, 6, 0x00008c8b8a898887)
	UNLDU(buf7, 7, 0x008d8c8b8a898887)
	UNLDU(buf7, 8, 0x8e8d8c8b8a898887)
	 UNST(str0, 1, 0x0000000000000010, 0x0000000000000000)
	 UNST(str0, 2, 0x0000000000003210, 0x0000000000000000)
	 UNST(str0, 3, 0x0000000000543210, 0x0000000000000000)
	 UNST(str0, 4, 0x0000000076543210, 0x0000000000000000)
	 UNST(str0, 5, 0x0000009876543210, 0x0000000000000000)
	 UNST(str0, 6, 0x0000ba9876543210, 0x0000000000000000)
	 UNST(str0, 7, 0x00dcba9876543210, 0x0000000000000000)
	 UNST(str0, 8, 0xfedcba9876543210, 0x0000000000000000)
	 UNST(str1, 1, 0x0000000000001000, 0x0000000000000000)
	 UNST(str1, 2, 0x0000000000321000, 0x0000000000000000)
	 UNST(str1, 3, 0x0000000054321000, 0x0000000000000000)
	 UNST(str1, 4, 0x0000007654321000, 0x0000000000000000)
	 UNST(str1, 5, 0x0000987654321000, 0x0000000000000000)
	 UNST(str1, 6, 0x00ba987654321000, 0x0000000000000000)
	 UNST(str1, 7, 0xdcba987654321000, 0x0000000000000000)
	 UNST(str1, 8, 0xdcba987654321000, 0x00000000000000fe)
	 UNST(str2, 1, 0x0000000000100000, 0x0000000000000000)
	 UNST(str2, 2, 0x0000000032100000, 0x0000000000000000)
	 UNST(str2, 3, 0x0000005432100000, 0x0000000000000000)
	 UNST(str2, 4, 0x0000765432100000, 0x0000000000000000)
	 UNST(str2, 5, 0x0098765432100000, 0x0000000000000000)
	 UNST(str2, 6, 0xba98765432100000, 0x0000000000000000)
	 UNST(str2, 7, 0xba98765432100000, 0x00000000000000dc)
	 UNST(str2, 8, 0xba98765432100000, 0x000000000000fedc)
	 UNST(str3, 1, 0x0000000010000000, 0x0000000000000000)
	 UNST(str3, 2, 0x0000003210000000, 0x0000000000000000)
	 UNST(str3, 3, 0x0000543210000000, 0x0000000000000000)
	 UNST(str3, 4, 0x0076543210000000, 0x0000000000000000)
	 UNST(str3, 5, 0x9876543210000000, 0x0000000000000000)
	 UNST(str3, 6, 0x9876543210000000, 0x00000000000000ba)
	 UNST(str3, 7, 0x9876543210000000, 0x000000000000dcba)
	 UNST(str3, 8, 0x9876543210000000, 0x0000000000fedcba)
	 UNST(str4, 1, 0x0000001000000000, 0x0000000000000000)
	 UNST(str4, 2, 0x0000321000000000, 0x0000000000000000)
	 UNST(str4, 3, 0x0054321000000000, 0x0000000000000000)
	 UNST(str4, 4, 0x7654321000000000, 0x0000000000000000)
	 UNST(str4, 5, 0x7654321000000000, 0x0000000000000098)
	 UNST(str4, 6, 0x7654321000000000, 0x000000000000ba98)
	 UNST(str4, 7, 0x7654321000000000, 0x0000000000dcba98)
	 UNST(str4, 8, 0x7654321000000000, 0x00000000fedcba98)
	 UNST(str5, 1, 0x0000100000000000, 0x0000000000000000)
	 UNST(str5, 2, 0x0032100000000000, 0x0000000000000000)
	 UNST(str5, 3, 0x5432100000000000, 0x0000000000000000)
	 UNST(str5, 4, 0x5432100000000000, 0x0000000000000076)
	 UNST(str5, 5, 0x5432100000000000, 0x0000000000009876)
	 UNST(str5, 6, 0x5432100000000000, 0x0000000000ba9876)
	 UNST(str5, 7, 0x5432100000000000, 0x00000000dcba9876)
	 UNST(str5, 8, 0x5432100000000000, 0x000000fedcba9876)
	 UNST(str6, 1, 0x0010000000000000, 0x0000000000000000)
	 UNST(str6, 2, 0x3210000000000000, 0x0000000000000000)
	 UNST(str6, 3, 0x3210000000000000, 0x0000000000000054)
	 UNST(str6, 4, 0x3210000000000000, 0x0000000000007654)
	 UNST(str6, 5, 0x3210000000000000, 0x0000000000987654)
	 UNST(str6, 6, 0x3210000000000000, 0x00000000ba987654)
	 UNST(str6, 7, 0x3210000000000000, 0x000000dcba987654)
	 UNST(str6, 8, 0x3210000000000000, 0x0000fedcba987654)
	 UNST(str7, 1, 0x1000000000000000, 0x0000000000000000)
	 UNST(str7, 2, 0x1000000000000000, 0x0000000000000032)
	 UNST(str7, 3, 0x1000000000000000, 0x0000000000005432)
	 UNST(str7, 4, 0x1000000000000000, 0x0000000000765432)
	 UNST(str7, 5, 0x1000000000000000, 0x0000000098765432)
	 UNST(str7, 6, 0x1000000000000000, 0x000000ba98765432)
	 UNST(str7, 7, 0x1000000000000000, 0x0000dcba98765432)
	 UNST(str7, 8, 0x1000000000000000, 0x00fedcba98765432)
#    else
	 UNLD(buf0, 1, 0xffffffffffffff80)
	 UNLD(buf0, 2, 0xffffffffffff8081)
	 UNLD(buf0, 3, 0xffffffffff808182)
	 UNLD(buf0, 4, 0xffffffff80818283)
	 UNLD(buf0, 5, 0xffffff8081828384)
	 UNLD(buf0, 6, 0xffff808182838485)
	 UNLD(buf0, 7, 0xff80818283848586)
	 UNLD(buf0, 8, 0x8081828384858687)
	 UNLD(buf1, 1, 0xffffffffffffff81)
	 UNLD(buf1, 2, 0xffffffffffff8182)
	 UNLD(buf1, 3, 0xffffffffff818283)
	 UNLD(buf1, 4, 0xffffffff81828384)
	 UNLD(buf1, 5, 0xffffff8182838485)
	 UNLD(buf1, 6, 0xffff818283848586)
	 UNLD(buf1, 7, 0xff81828384858687)
	 UNLD(buf1, 8, 0x8182838485868788)
	 UNLD(buf2, 1, 0xffffffffffffff82)
	 UNLD(buf2, 2, 0xffffffffffff8283)
	 UNLD(buf2, 3, 0xffffffffff828384)
	 UNLD(buf2, 4, 0xffffffff82838485)
	 UNLD(buf2, 5, 0xffffff8283848586)
	 UNLD(buf2, 6, 0xffff828384858687)
	 UNLD(buf2, 7, 0xff82838485868788)
	 UNLD(buf2, 8, 0x8283848586878889)
	 UNLD(buf3, 1, 0xffffffffffffff83)
	 UNLD(buf3, 2, 0xffffffffffff8384)
	 UNLD(buf3, 3, 0xffffffffff838485)
	 UNLD(buf3, 4, 0xffffffff83848586)
	 UNLD(buf3, 5, 0xffffff8384858687)
	 UNLD(buf3, 6, 0xffff838485868788)
	 UNLD(buf3, 7, 0xff83848586878889)
	 UNLD(buf3, 8, 0x838485868788898a)
	 UNLD(buf4, 1, 0xffffffffffffff84)
	 UNLD(buf4, 2, 0xffffffffffff8485)
	 UNLD(buf4, 3, 0xffffffffff848586)
	 UNLD(buf4, 4, 0xffffffff84858687)
	 UNLD(buf4, 5, 0xffffff8485868788)
	 UNLD(buf4, 6, 0xffff848586878889)
	 UNLD(buf4, 7, 0xff8485868788898a)
	 UNLD(buf4, 8, 0x8485868788898a8b)
	 UNLD(buf5, 1, 0xffffffffffffff85)
	 UNLD(buf5, 2, 0xffffffffffff8586)
	 UNLD(buf5, 3, 0xffffffffff858687)
	 UNLD(buf5, 4, 0xffffffff85868788)
	 UNLD(buf5, 5, 0xffffff8586878889)
	 UNLD(buf5, 6, 0xffff85868788898a)
	 UNLD(buf5, 7, 0xff85868788898a8b)
	 UNLD(buf5, 8, 0x85868788898a8b8c)
	 UNLD(buf6, 1, 0xffffffffffffff86)
	 UNLD(buf6, 2, 0xffffffffffff8687)
	 UNLD(buf6, 3, 0xffffffffff868788)
	 UNLD(buf6, 4, 0xffffffff86878889)
	 UNLD(buf6, 5, 0xffffff868788898a)
	 UNLD(buf6, 6, 0xffff868788898a8b)
	 UNLD(buf6, 7, 0xff868788898a8b8c)
	 UNLD(buf6, 8, 0x868788898a8b8c8d)
	 UNLD(buf7, 1, 0xffffffffffffff87)
	 UNLD(buf7, 2, 0xffffffffffff8788)
	 UNLD(buf7, 3, 0xffffffffff878889)
	 UNLD(buf7, 4, 0xffffffff8788898a)
	 UNLD(buf7, 5, 0xffffff8788898a8b)
	 UNLD(buf7, 6, 0xffff8788898a8b8c)
	 UNLD(buf7, 7, 0xff8788898a8b8c8d)
	 UNLD(buf7, 8, 0x8788898a8b8c8d8e)
	UNLDU(buf0, 1, 0x0000000000000080)
	UNLDU(buf0, 2, 0x0000000000008081)
	UNLDU(buf0, 3, 0x0000000000808182)
	UNLDU(buf0, 4, 0x0000000080818283)
	UNLDU(buf0, 5, 0x0000008081828384)
	UNLDU(buf0, 6, 0x0000808182838485)
	UNLDU(buf0, 7, 0x0080818283848586)
	UNLDU(buf0, 8, 0x8081828384858687)
	UNLDU(buf1, 1, 0x0000000000000081)
	UNLDU(buf1, 2, 0x0000000000008182)
	UNLDU(buf1, 3, 0x0000000000818283)
	UNLDU(buf1, 4, 0x0000000081828384)
	UNLDU(buf1, 5, 0x0000008182838485)
	UNLDU(buf1, 6, 0x0000818283848586)
	UNLDU(buf1, 7, 0x0081828384858687)
	UNLDU(buf1, 8, 0x8182838485868788)
	UNLDU(buf2, 1, 0x0000000000000082)
	UNLDU(buf2, 2, 0x0000000000008283)
	UNLDU(buf2, 3, 0x0000000000828384)
	UNLDU(buf2, 4, 0x0000000082838485)
	UNLDU(buf2, 5, 0x0000008283848586)
	UNLDU(buf2, 6, 0x0000828384858687)
	UNLDU(buf2, 7, 0x0082838485868788)
	UNLDU(buf2, 8, 0x8283848586878889)
	UNLDU(buf3, 1, 0x0000000000000083)
	UNLDU(buf3, 2, 0x0000000000008384)
	UNLDU(buf3, 3, 0x0000000000838485)
	UNLDU(buf3, 4, 0x0000000083848586)
	UNLDU(buf3, 5, 0x0000008384858687)
	UNLDU(buf3, 6, 0x0000838485868788)
	UNLDU(buf3, 7, 0x0083848586878889)
	UNLDU(buf3, 8, 0x838485868788898a)
	UNLDU(buf4, 1, 0x0000000000000084)
	UNLDU(buf4, 2, 0x0000000000008485)
	UNLDU(buf4, 3, 0x0000000000848586)
	UNLDU(buf4, 4, 0x0000000084858687)
	UNLDU(buf4, 5, 0x0000008485868788)
	UNLDU(buf4, 6, 0x0000848586878889)
	UNLDU(buf4, 7, 0x008485868788898a)
	UNLDU(buf4, 8, 0x8485868788898a8b)
	UNLDU(buf5, 1, 0x0000000000000085)
	UNLDU(buf5, 2, 0x0000000000008586)
	UNLDU(buf5, 3, 0x0000000000858687)
	UNLDU(buf5, 4, 0x0000000085868788)
	UNLDU(buf5, 5, 0x0000008586878889)
	UNLDU(buf5, 6, 0x000085868788898a)
	UNLDU(buf5, 7, 0x0085868788898a8b)
	UNLDU(buf5, 8, 0x85868788898a8b8c)
	UNLDU(buf6, 1, 0x0000000000000086)
	UNLDU(buf6, 2, 0x0000000000008687)
	UNLDU(buf6, 3, 0x0000000000868788)
	UNLDU(buf6, 4, 0x0000000086878889)
	UNLDU(buf6, 5, 0x000000868788898a)
	UNLDU(buf6, 6, 0x0000868788898a8b)
	UNLDU(buf6, 7, 0x00868788898a8b8c)
	UNLDU(buf6, 8, 0x868788898a8b8c8d)
	UNLDU(buf7, 1, 0x0000000000000087)
	UNLDU(buf7, 2, 0x0000000000008788)
	UNLDU(buf7, 3, 0x0000000000878889)
	UNLDU(buf7, 4, 0x000000008788898a)
	UNLDU(buf7, 5, 0x0000008788898a8b)
	UNLDU(buf7, 6, 0x00008788898a8b8c)
	UNLDU(buf7, 7, 0x008788898a8b8c8d)
	UNLDU(buf7, 8, 0x8788898a8b8c8d8e)
	 UNST(str0, 1, 0x1000000000000000, 0x0000000000000000)
	 UNST(str0, 2, 0x3210000000000000, 0x0000000000000000)
	 UNST(str0, 3, 0x5432100000000000, 0x0000000000000000)
	 UNST(str0, 4, 0x7654321000000000, 0x0000000000000000)
	 UNST(str0, 5, 0x9876543210000000, 0x0000000000000000)
	 UNST(str0, 6, 0xba98765432100000, 0x0000000000000000)
	 UNST(str0, 7, 0xdcba987654321000, 0x0000000000000000)
	 UNST(str0, 8, 0xfedcba9876543210, 0x0000000000000000)
	 UNST(str1, 1, 0x0010000000000000, 0x0000000000000000)
	 UNST(str1, 2, 0x0032100000000000, 0x0000000000000000)
	 UNST(str1, 3, 0x0054321000000000, 0x0000000000000000)
	 UNST(str1, 4, 0x0076543210000000, 0x0000000000000000)
	 UNST(str1, 5, 0x0098765432100000, 0x0000000000000000)
	 UNST(str1, 6, 0x00ba987654321000, 0x0000000000000000)
	 UNST(str1, 7, 0x00dcba9876543210, 0x0000000000000000)
	 UNST(str1, 8, 0x00fedcba98765432, 0x1000000000000000)
	 UNST(str2, 1, 0x0000100000000000, 0x0000000000000000)
	 UNST(str2, 2, 0x0000321000000000, 0x0000000000000000)
	 UNST(str2, 3, 0x0000543210000000, 0x0000000000000000)
	 UNST(str2, 4, 0x0000765432100000, 0x0000000000000000)
	 UNST(str2, 5, 0x0000987654321000, 0x0000000000000000)
	 UNST(str2, 6, 0x0000ba9876543210, 0x0000000000000000)
	 UNST(str2, 7, 0x0000dcba98765432, 0x1000000000000000)
	 UNST(str2, 8, 0x0000fedcba987654, 0x3210000000000000)
	 UNST(str3, 1, 0x0000001000000000, 0x0000000000000000)
	 UNST(str3, 2, 0x0000003210000000, 0x0000000000000000)
	 UNST(str3, 3, 0x0000005432100000, 0x0000000000000000)
	 UNST(str3, 4, 0x0000007654321000, 0x0000000000000000)
	 UNST(str3, 5, 0x0000009876543210, 0x0000000000000000)
	 UNST(str3, 6, 0x000000ba98765432, 0x1000000000000000)
	 UNST(str3, 7, 0x000000dcba987654, 0x3210000000000000)
	 UNST(str3, 8, 0x000000fedcba9876, 0x5432100000000000)
	 UNST(str4, 1, 0x0000000010000000, 0x0000000000000000)
	 UNST(str4, 2, 0x0000000032100000, 0x0000000000000000)
	 UNST(str4, 3, 0x0000000054321000, 0x0000000000000000)
	 UNST(str4, 4, 0x0000000076543210, 0x0000000000000000)
	 UNST(str4, 5, 0x0000000098765432, 0x1000000000000000)
	 UNST(str4, 6, 0x00000000ba987654, 0x3210000000000000)
	 UNST(str4, 7, 0x00000000dcba9876, 0x5432100000000000)
	 UNST(str4, 8, 0x00000000fedcba98, 0x7654321000000000)
	 UNST(str5, 1, 0x0000000000100000, 0x0000000000000000)
	 UNST(str5, 2, 0x0000000000321000, 0x0000000000000000)
	 UNST(str5, 3, 0x0000000000543210, 0x0000000000000000)
	 UNST(str5, 4, 0x0000000000765432, 0x1000000000000000)
	 UNST(str5, 5, 0x0000000000987654, 0x3210000000000000)
	 UNST(str5, 6, 0x0000000000ba9876, 0x5432100000000000)
	 UNST(str5, 7, 0x0000000000dcba98, 0x7654321000000000)
	 UNST(str5, 8, 0x0000000000fedcba, 0x9876543210000000)
	 UNST(str6, 1, 0x0000000000001000, 0x0000000000000000)
	 UNST(str6, 2, 0x0000000000003210, 0x0000000000000000)
	 UNST(str6, 3, 0x0000000000005432, 0x1000000000000000)
	 UNST(str6, 4, 0x0000000000007654, 0x3210000000000000)
	 UNST(str6, 5, 0x0000000000009876, 0x5432100000000000)
	 UNST(str6, 6, 0x000000000000ba98, 0x7654321000000000)
	 UNST(str6, 7, 0x000000000000dcba, 0x9876543210000000)
	 UNST(str6, 8, 0x000000000000fedc, 0xba98765432100000)
	 UNST(str7, 1, 0x0000000000000010, 0x0000000000000000)
	 UNST(str7, 2, 0x0000000000000032, 0x1000000000000000)
	 UNST(str7, 3, 0x0000000000000054, 0x3210000000000000)
	 UNST(str7, 4, 0x0000000000000076, 0x5432100000000000)
	 UNST(str7, 5, 0x0000000000000098, 0x7654321000000000)
	 UNST(str7, 6, 0x00000000000000ba, 0x9876543210000000)
	 UNST(str7, 7, 0x00000000000000dc, 0xba98765432100000)
	 UNST(str7, 8, 0x00000000000000fe, 0xdcba987654321000)
#    endif

#  endif
	FLDST(str0, 2.5)
	FLDST(str1, 2.5)
	FLDST(str2, 2.5)
	FLDST(str3, 2.5)
	FLDST(str4, 2.5)
	DLDST(str0, 7.5)
	DLDST(str1, 7.5)
	DLDST(str2, 7.5)
	DLDST(str3, 7.5)
	DLDST(str4, 7.5)
	DLDST(str5, 7.5)
	DLDST(str6, 7.5)
	DLDST(str7, 7.5)
	DLDST(str8, 7.5)

	prepare
		pushargi ok
	finishi @puts
#endif
	ret
	epilog
