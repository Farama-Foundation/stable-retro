.data	256

/*
	#define offs(field)	(offsetof(data_t, field) - offsetof(data_t, si8))
 */
#define nF32		-36	// offs(nf32)
#define nF64		-32	// offs(nf64)
#define nSI64		-24	// offs(nsi64)
#define nUI32		-16	// offs(nui32)
#define nSI32		-12	// offs(nsi32)
#define nUI16		 -6	// offs(nui16)
#define nSI16		 -4	// offs(nsi16)
#define nUI8		 -2	// offs(nui8)
#define nSI8		 -1	// offs(nsi8)
#define SI8		  0	// offs(si8)
#define UI8		  1	// offs(ui8)
#define SI16		  2	// offs(si18)
#define UI16		  4	// offs(ui16)
#define SI32		  8	// offs(si32)
#define UI32		 12	// offs(ui32)
#define SI64		 16	// offs(si64)
#define F64		 24	// offs(f64)
#define F32		 32	// offs(f32)

/*
	typedef struct {
		int32_t		_pad0;
		float32_t	nf32;
		float64_t	nf64;
		int64_t		nsi64;
		uint32_t	nui32;
		int32_t		nsi32;
		short		_pad1;
		uint16_t	nui16;
		int16_6		nsi16;
		uint8_t		nui8;
		int8_t		nsi8;
		int8_t		si8;
		uint8_t		ui8;
		int16_t		si16;
		uint16_t	ui16;
		int16_t		_pad2;
		int32_t		si32;
		uint32_t	ui32;
		int64_t		si64;
		float64_t	f64;
		float32_t	f32;
		int32_t		_pad3;
	} data_t;
	data_t			data;
 */

data:
.size	4
minus_thirty_six:		// nF32
.size	4
minus_thirty_two:		// nF64
.size	8
minus_twenty_four:		// nSI64
.size	8
minus_sixteen:			// nUI32
.size	4
minus_twelve:			// nSI32
.size	4
.size	2			// pad
minus_six:			// nUI16
.size	2
minus_four:			// nSI16
.size	2
minus_two:			// nUI8
.size	1
minus_one:
.size	1			// nSI8
zero:				// SI8
.size	1
one:				// UI8
.size	1
two:				// SI16
.size	2
four:				// UI16
.size	2
.size	2			// pad
eight:				// SI32
.size	4
twelve:				// UI32
.size	4
sixteen:			// SI64
.size	8
twenty_four:			// F64
.size	8
thirty_two:			// F32
.size	4
thirty_six:
.align	8
/*
	data_t			buffer;
 */
buffer:
.size	80

ok:
.c	"ok"

.code
	jmpi main

/*
	void reset(void) {
		memset(data, -1, sizeof(data));
		data.nf32  = nF32;
		data.nf64  = nF64;
	#if __WORDSIZE == 64
		data.nsi64 = nSI64;
		data.nui32 = nUI32;
	#endif
		data.nsi32 = nSI32;
		data.nui16 = nUI16;
		data.nsi16 = nSI16;
		data.nui8  = nUI8;
		data.nsi8  = nSI8;
		data.si8   = SI8;
		data.ui8   = UI8;
		data.si16  = SI16;
		data.ui16  = UI16;
		data.si32  = SI32;
	#if __WORDSIZE == 64
		data.ui32  = UI32;
		data.si64  = SI64;
	#endif
		data.f64   = F64;
		data.f32   = F32;
	}
 */
reset:
	prolog
	movi %v0  data
	prepare
		pushargr %v0
		pushargi -1
		pushargi 80
	finishi @memset
	addi %v0 %v0 4
	movi_f %f0 nF32
	str_f %v0 %f0
	addi %v0 %v0 $(nF64 - nF32)
	movi_d %f0 nF64
	str_d %v0 %f0
	addi %v0 %v0 $(nSI64 - nF64)	
	movi %r0 nSI64
#if __WORDSIZE == 64
	str_l %v0 %r0
#endif
	addi %v0 %v0 $(nUI32 - nSI64)
	movi %r0 nUI32
#if __WORDSIZE == 64
	str_i %v0 %r0
#endif
	addi %v0 %v0 $(nSI32 - nUI32)
	movi %r0 nSI32
	str_i %v0 %r0
	addi %v0 %v0 $(nUI16 - nSI32)
	movi %r0 nUI16
	str_s %v0 %r0
	addi %v0 %v0 $(nSI16 - nUI16)
	movi %r0 nSI16
	str_s %v0 %r0
	addi %v0 %v0 $(nUI8 - nSI16)
	movi %r0 nUI8
	str_c %v0 %r0
	addi %v0 %v0 $(nSI8 - nUI8)
	movi %r0 nSI8
	str_c %v0 %r0
	addi %v0 %v0 $(SI8 - nSI8)
	movi %r0 SI8
	str_c %v0 %r0
	addi %v0 %v0 $(UI8 - SI8)
	movi %r0 UI8
	str_c %v0 %r0
	addi %v0 %v0 $(SI16 - UI8)
	movi %r0 SI16
	str_s %v0 %r0
	addi %v0 %v0 $(UI16 - SI16)
	movi %r0 UI16
	str_s %v0 %r0
	addi %v0 %v0 $(SI32 - UI16)
	movi %r0 SI32
	str_i %v0 %r0
	addi %v0 %v0 $(UI32 - SI32)
	movi %r0 UI32
#if __WORDSIZE == 64
	str_i %v0 %r0
#endif
	addi %v0 %v0 $(SI64 - UI32)
	movi %r0 SI64
#if __WORDSIZE == 64
	str_l %v0 %r0
#endif
	addi %v0 %v0 $(F64 - SI64)
	movi_d %f0 F64
	str_d %v0 %f0
	addi %v0 %v0 $(F32 - F64)
	movi_f %f0 F32
	str_f %v0 %f0
	ret
	epilog

#if __WORDSIZE == 64
#  define IF32(expr)			/**/
#  define IF64(expr)			expr
#else
#  define IF32(expr)			expr
#  define IF64(expr)			/**/
#endif

/*
	union {
		int8_t		*i8;
		uint8_t		*u8;
		int16_t		*i16;
		uint16_t	*u16;
		int32_t		*i32;
		uint32_t	*u32;
		int64_t		*i64;
		float32_t	*f32;
		float64_t	*f64;
	} u;
	reset();
	u.i8 = (char *)data + offsetof(data_t, si8);
	if (*--u.i8  != nSI8)		goto fail;
	if (*--u.u8  != nUI8)		goto fail;
	if (*--u.i16 != nSI16)		goto fail;
	if (*--u.u16 != nUI16)		goto fail;
	--u.nsi16;
	if (*--u.i32 != nSI32)		goto fail;
#if __WORDSIZE == 64
	if (*--u.u32 != nUI32)		goto fail;
	if (*--u.i64 != nSI64)		goto fail;
#else
	u.i8 -= 12;
#endif
	if (*--u.f64 != nF64)		goto fail;
	if (*--u.f32 != nF32)		goto fail;
	u.i8 = (char *)data + offsetof(data_t, si8);
	if (*u.i8++  != SI8)		goto fail;
	if (*u.u8++  != UI8)		goto fail;
	if (*u.i16++ != SI16)		goto fail;
	if (*u.u16++ != UI16)		goto fail;
	++u.i16;
	if (*u.i32++ != SI32)		goto fail;
#if __WORDSIZE == 64
	if (*u.u32++ != UI32)		goto fail;
	if (*u.i64++ != SI64)		goto fail;
#else
	u.i8 += 12;
#endif
	if (*u.f64++ != F64)		goto fail;
	if (*u.f32++ != F32)		goto fail;
	goto done;
fail:
	abort();
done:
	memset(buffer, -1, 80);
	u.i8 = (char *)buffer + offsetof(data_t, si8);
	*--u.i8  = nSI8;
	*--u.u8  = nUI8;
	*--u.i16 = nSI16;
	*--u.u16 = nUI16;
	--u.i16;
	*--u.i32 = nSI32;
#if __WORDSIZE == 64
	*--u.u32 = nUI32;
	*--u.i64 = nSI64;
#else
	u.i8 -= 12;
#endif
	*--u.f64 = nF64;
	*--u.f32 = nF32;
	u.i8 = (char *)buffer + offsetof(data_t, si8);
	u.i8++  = SI8;
	u.u8++  = UI8;
	u.i16++ = SI16;
	u.u16++ = UI16;
	++u.i16;
	u.i32++ = SI32;
#if __WORDSIZE == 64
	u.u32++ = UI32;
	u.i64++ = SI64;
#else
	u.i8 += 12;
#endif
	u.f64++ = F64;
	u.f32++ = F32;
	if (memcp(buffer, data, sizeof(data_t)))
		abort();
 */
#define TEST(R0, F0, R1)						\
	calli reset							\
	movi %R1 zero							\
	ldxbi_c %R0 %R1 $(nSI8 - SI8)					\
	bnei fail##R0##F0##R1 %R0 nSI8					\
	ldxbi_uc %R0 %R1 $(nUI8 - nSI8)					\
	extr_c %R0 %R0							\
	bnei fail##R0##F0##R1 %R0 nUI8					\
	ldxbi_s %R0 %R1 $(nSI16 - nUI8)					\
	bnei fail##R0##F0##R1 %R0 nSI16					\
	ldxbi_us %R0 %R1 $(nUI16 - nSI16)				\
	extr_s %R0 %R0							\
	bnei fail##R0##F0##R1 %R0 nUI16					\
	ldxbi_i %R0 %R1 $(nSI32 - nUI16)				\
	bnei fail##R0##F0##R1 %R0 nSI32					\
	IF64(ldxbi_ui %R0 %R1 $(nUI32 - nSI32))				\
	IF64(extr_i %R0 %R0)						\
	IF64(bnei fail##R0##F0##R1 %R0 nUI32)				\
	IF32(addi %R1 %R1 $(nUI32 - nSI32))				\
	IF64(ldxbi_l %R0 %R1 $(nSI64 - nUI32))				\
	IF64(bnei fail##R0##F0##R1 %R0 nSI64)				\
	IF32(addi %R1 %R1 $(nSI64 - nUI32))				\
	ldxbi_d %F0 %R1 $(nF64 - nSI64)					\
	bnei_d fail##R0##F0##R1 %F0 nF64				\
	ldxbi_f %F0 %R1 $(nF32 - nF64)					\
	bnei_f fail##R0##F0##R1 %F0 nF32				\
	movi %R1 zero							\
	ldxai_c %R0 %R1 $(UI8 - SI8)					\
	bnei fail##R0##F0##R1 %R0 SI8					\
	ldxai_uc %R0 %R1 $(SI16 - UI8)					\
	bnei fail##R0##F0##R1 %R0 UI8					\
	ldxai_s %R0 %R1 $(UI16 - SI16)					\
	bnei fail##R0##F0##R1 %R0 SI16					\
	ldxai_us %R0 %R1 $(SI32 - UI16)					\
	bnei fail##R0##F0##R1 %R0 UI16					\
	ldxai_i %R0 %R1 $(UI32 - SI32)					\
	bnei fail##R0##F0##R1 %R0 SI32					\
	IF64(ldxai_ui %R0 %R1 $(SI64 - UI32))				\
	IF64(bnei fail##R0##F0##R1 %R0 UI32)				\
	IF32(addi %R1 %R1 $(SI64 - UI32))				\
	IF64(ldxai_l %R0 %R1 $(F64 - SI64))				\
	IF64(bnei fail##R0##F0##R1 %R0 SI64)				\
	IF32(addi %R1 %R1 $(F64 - SI64))				\
	ldxai_d %F0 %R1 $(F32 - F64)					\
	bnei_d fail##R0##F0##R1 %F0 F64					\
	ldxai_f %F0 %R1 $(36 - F32)					\
	bnei_f fail##R0##F0##R1 %F0 F32					\
	jmpi done##R0##F0##R1						\
fail##R0##F0##R1:							\
	calli @abort							\
done##R0##F0##R1:							\
	prepare								\
		pushargi buffer						\
		pushargi -1						\
		pushargi 80						\
	finishi @memset							\
	movi %R1 buffer							\
	addi %R1 %R1 40							\
	movi %R0 nSI8							\
	stxbi_c $(nSI8 - SI8) %R1 %R0					\
	movi %R0 nUI8							\
	extr_uc %R0 %R0							\
	stxbi_c $(nUI8 - nSI8) %R1 %R0					\
	movi %R0 nSI16							\
	stxbi_s $(nSI16 - nUI8) %R1 %R0					\
	movi %R0 nUI16							\
	extr_us	%R0 %R0							\
	stxbi_s $(nUI16 - nSI16) %R1 %R0 				\
	movi %R0 nSI32							\
	stxbi_i $(nSI32 - nUI16) %R1 %R0 				\
	IF64(movi %R0 nUI32)						\
	IF64(stxbi_i $(nUI32 - nSI32) %R1 %R0)				\
	IF32(addi %R1 %R1 $(nUI32 - nSI32))				\
	IF64(movi %R0 nSI64)						\
	IF64(stxbi_l $(nSI64 - nUI32) %R1 %R0)				\
	IF32(addi %R1 %R1 $(nSI64 - nUI32))				\
	movi_d %F0 nF64							\
	stxbi_d $(nF64 - nSI64) %R1 %F0					\
	movi_f %F0 nF32							\
	stxbi_f $(nF32 - nF64) %R1 %F0					\
	movi %R1 buffer							\
	addi %R1 %R1 40							\
	movi %R0 SI8							\
	stxai_c $(UI8 - SI8) %R1 %R0					\
	movi %R0 UI8							\
	stxai_c $(SI16 - UI8) %R1 %R0					\
	movi %R0 SI16							\
	stxai_s $(UI16 - SI16) %R1 %R0					\
	movi %R0 UI16							\
	stxai_s $(SI32 - UI16) %R1 %R0					\
	movi %R0 SI32							\
	stxai_i $(UI32 - SI32) %R1 %R0					\
	IF64(movi %R0 UI32)						\
	IF64(stxai_i $(SI64 - UI32) %R1 %R0)				\
	IF32(addi %R1 %R1 $(SI64 - UI32))				\
	IF64(movi %R0 SI64)						\
	IF64(stxai_l $(F64 - SI64) %R1 %R0)				\
	IF32(addi %R1 %R1 $(F64 - SI64))				\
	movi_d %F0 F64							\
	stxai_d $(F32 - F64) %R1 %F0					\
	movi_f %F0 F32							\
	stxai_f $(36 - F32) %R1 %F0					\
	prepare								\
		pushargi data						\
		pushargi buffer						\
		pushargi 80						\
	finishi @memcmp							\
	retval %R0							\
	beqi done2##R0##F0##R1 %R0 0					\
	calli @abort							\
done2##R0##F0##R1:

main:
	prolog
	TEST(r0, f0, r1)
	TEST(r0, f0, r2)
	TEST(r0, f0, v0)
	TEST(r0, f0, v1)
	TEST(r0, f0, v2)
	TEST(r1, f1, r0)
	TEST(r1, f1, r2)
	TEST(r1, f1, v0)
	TEST(r1, f1, v1)
	TEST(r1, f1, v2)
	TEST(r2, f2, r0)
	TEST(r2, f2, r1)
	TEST(r2, f2, v0)
	TEST(r2, f2, v1)
	TEST(r2, f2, v2)
	TEST(v0, f3, r0)
	TEST(v0, f3, r1)
	TEST(v0, f3, r2)
	TEST(v0, f3, v1)
	TEST(v0, f3, v2)
	TEST(v1, f4, r0)
	TEST(v1, f4, r1)
	TEST(v1, f4, r2)
	TEST(v1, f4, v0)
	TEST(v1, f4, v2)
	TEST(v2, f5, r0)
	TEST(v2, f5, r1)
	TEST(v2, f5, r2)
	TEST(v2, f5, v0)
	TEST(v2, f5, v1)
	prepare
		pushargi ok
	finishi @puts
	ret
	epilog
