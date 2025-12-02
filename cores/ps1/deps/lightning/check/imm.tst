.data		4096
#if __WORDSIZE == 32
fmt_i:
.c		"%s 0x%x = 0x%x (expected 0x%x)\n"
fmt_ext:
.c		"%s 0x%x %d %d = 0x%x (expected 0x%x)\n"
fmt_dep:
.c		"depi 0x%x 0x%x %d %d = 0x%x (expected 0x%x)\n"
#else
fmt_i:
.c		"%s 0x%lx = 0x%lx (expected 0x%lx)\n"
fmt_ext:
.c		"%s 0x%lx %ld %ld = 0x%lx (expected 0x%lx)\n"
fmt_dep:
.c		"depi 0x%lx 0x%lx %ld %ld = 0x%lx (expected 0x%lx)\n"
#endif
fmt_d:
.c		"%s %.12f = %.12f (expected %.12f)\n"
#define DEF(str)							\
S##str:									\
.c		#str
DEF(negi)
DEF(comi)
DEF(exti_c)
DEF(exti_uc)
DEF(exti_s)
DEF(exti_us)
DEF(exti_i)
DEF(exti_ui)
DEF(htoni_us)
DEF(ntohi_us)
DEF(htoni_ui)
DEF(ntohi_ui)
DEF(htoni_ul)
DEF(ntohi_ul)
DEF(htoni)
DEF(ntohi)
DEF(bswapi_us)
DEF(bswapi_ui)
DEF(bswapi_ul)
DEF(bswapi)
DEF(cloi)
DEF(clzi)
DEF(ctoi)
DEF(ctzi)
DEF(rbiti)
DEF(popcnti)
DEF(exti)
DEF(exti_u)
DEF(negi_f)
DEF(absi_f)
DEF(sqrti_f)
DEF(negi_d)
DEF(absi_d)
DEF(sqrti_d)
ok:
.c	"ok"

#define CHECKI(OP, I0, I1)						\
	OP %r0 I0							\
	beqi OP##_ok %r0 I1						\
	prepare								\
		pushargi fmt_i						\
		ellipsis						\
		pushargi S##OP						\
		pushargi I0						\
		pushargr %r0						\
		pushargi I1						\
	finishi @printf							\
	calli @abort							\
OP##_ok:
#define CHECKEXT(OP, I0, I1, I2, I3)					\
	OP %r0 I0 I1 I2							\
	beqi OP##_ok %r0 I3						\
	prepare								\
		pushargi fmt_ext					\
		ellipsis						\
		pushargi S##OP						\
		pushargi I0						\
		pushargi I1						\
		pushargi I2						\
		pushargr %r0						\
		pushargi I3						\
	finishi @printf							\
	calli @abort							\
OP##_ok:
#define CHECKDEP(I0, I1, I2, I3, I4)					\
	movi %r0 I0							\
	depi %r0 I1 I2 I3						\
	beqi dep_ok %r0 I4						\
	prepare								\
		pushargi fmt_dep					\
		ellipsis						\
		pushargi I0						\
		pushargi I1						\
		pushargi I2						\
		pushargi I3						\
		pushargr %r0						\
		pushargi I4						\
	finishi @printf							\
	calli @abort							\
dep_ok:
#define CHECKD(OP, I0, I1)						\
	OP %f0 I0							\
	beqi_d OP##_ok %f0 I1						\
	prepare								\
		pushargi fmt_d						\
		ellipsis						\
		pushargi S##OP						\
		pushargi_d I0						\
		pushargr_d %f0						\
		pushargi_d I1						\
	finishi @printf							\
	calli @abort							\
OP##_ok:
#define CHECKF(OP, I0, I1)						\
	OP %f0 I0							\
	beqi_f OP##_ok %f0 I1						\
	extr_f_d %f0 %f0						\
	prepare								\
		pushargi fmt_d						\
		ellipsis						\
		pushargi S##OP						\
		pushargi_d I0						\
		pushargr_d %f0						\
		pushargi_d I1						\
	finishi @printf							\
	calli @abort							\
OP##_ok:

.code
	prolog

	CHECKI(negi, 1, -1)
	CHECKI(comi, 0, -1)
	CHECKI(exti_c, 0xfff, -1)
	CHECKI(exti_uc, 0xfff, 0xff)
	CHECKI(exti_s, 0xfffff, -1)
	CHECKI(exti_us, 0xfffff, 0xffff)
#if __BYTE_ORDER == __BIG_ENDIAN
	CHECKI(htoni_us, 0xff1234, 0x1234)
	CHECKI(ntohi_us, 0x7ffff, 0xffff)
#else
	CHECKI(htoni_us, 0xff1234, 0x3412)
	CHECKI(ntohi_us, 0x7ffff, 0xffff)
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
#  if __WORDSIZE == 32
	CHECKI(htoni_ui, 0x12345678, 0x12345678)
	CHECKI(ntohi_ui, 0x78563412, 0x78563412)
#  else
	CHECKI(htoni_ui, 0x7f12345678, 0x12345678)
	CHECKI(ntohi_ui, 0xf778563412, 0x78563412)
#  endif
#else
#  if __WORDSIZE == 32
	CHECKI(htoni_ui, 0x12345678, 0x78563412)
	CHECKI(ntohi_ui, 0x78563412, 0x12345678)
#  else
	CHECKI(htoni_ui, 0x7f12345678, 0x78563412)
	CHECKI(ntohi_ui, 0xf778563412, 0x12345678)
#  endif
#endif
	CHECKI(bswapi_us, 0x1234, 0x3412)
	CHECKI(bswapi_ui, 0x12345678, 0x78563412)
#if __WORDSIZE == 32
#  if __BYTE_ORDER == __BIG_ENDIAN
	CHECKI(htoni, 0x78563412, 0x78563412)
	CHECKI(ntohi, 0x12345678, 0x12345678)
#  else
	CHECKI(htoni, 0x78563412, 0x12345678)
	CHECKI(ntohi, 0x12345678, 0x78563412)
#  endif
	CHECKI(bswapi, 0x78563412, 0x12345678)
#else
#  if __BYTE_ORDER == __BIG_ENDIAN
	CHECKI(htoni_ul, 0xf0debc9a78563412, 0xf0debc9a78563412)
	CHECKI(ntohi_ul, 0x123456789abcdef0, 0x123456789abcdef0)
	CHECKI(htoni, 0x123456789abcdef0, 0x123456789abcdef0)
	CHECKI(ntohi, 0xf0debc9a78563412, 0xf0debc9a78563412)
#  else
	CHECKI(htoni_ul, 0x123456789abcdef0, 0xf0debc9a78563412)
	CHECKI(ntohi_ul, 0xf0debc9a78563412, 0x123456789abcdef0)
	CHECKI(htoni, 0xf0debc9a78563412, 0x123456789abcdef0)
	CHECKI(ntohi, 0x123456789abcdef0, 0xf0debc9a78563412)
#  endif
	CHECKI(exti_i, 0x80000000, 0xffffffff80000000)
	CHECKI(exti_ui, 0x80000000, 0x80000000)
	CHECKI(bswapi_ul, 0x123456789abcdef0, 0xf0debc9a78563412)
	CHECKI(bswapi, 0xf0debc9a78563412, 0x123456789abcdef0)
#endif
#if __WORDSIZE == 32
	CHECKI(cloi, 0xfffffffe, 31)
	CHECKI(clzi, 1, 31)
	CHECKI(ctoi, 0x7fffffff, 31)
	CHECKI(ctzi, 0x80000000, 31)
	CHECKI(rbiti, 0x02468ace, 0x73516240)
	CHECKI(popcnti, 0x8a13c851, 12)
#else
	CHECKI(cloi, 0xfffffffffffffffe, 63)
	CHECKI(clzi, 1, 63)
	CHECKI(ctoi, 0x7fffffffffffffff, 63)
	CHECKI(ctzi, 0x8000000000000000, 63)
	CHECKI(rbiti, 0x02468ace013579bd, 0xbd9eac8073516240)
	CHECKI(popcnti, 0x02468ace013579bd, 28)
#endif
#if __WORDSIZE == 32
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	CHECKEXT(exti, 0xa5a5a584, 1, 2, 0xfffffffe)
	CHECKEXT(exti_u, 0xa5a5a584, 1, 2, 0x00000002)
	CHECKDEP(0xa5a5a584, 1, 1, 2, 0xa5a5a582)
#  else
	CHECKEXT(exti, 0xa5a5a3b7, 29, 1, 0xffffffff)
	CHECKEXT(exti_u, 0xa5a5a3b7, 29, 1, 0x00000001)
	CHECKDEP(0xa5a5a3b7, 0, 29, 1, 0xa5a5a3b3)
#  endif
#else
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	CHECKEXT(exti, 0xa5a5a5a5a5a5a564, 1, 2, 0xfffffffffffffffe)
	CHECKEXT(exti_u, 0xa5a5a5a5a5a5a564, 1, 2, 0x0000000000000002)
	CHECKDEP(0xa5a5a5a5a5a5a564, 1, 1, 2, 0xa5a5a5a5a5a5a562)
#  else
	CHECKEXT(exti, 0xa5a5a5a5a5a59dc8, 60, 3, 0xfffffffffffffffc)
	CHECKEXT(exti_u, 0xa5a5a5a5a5a59dc6, 61, 2, 0x0000000000000002)
	CHECKDEP(0xa5a5a5a5a5a59dc6, 1, 61, 2, 0xa5a5a5a5a5a59dc2)
#  endif
#endif
	CHECKF(negi_f, 2.0, -2.0)
	CHECKF(absi_f, -3.0, 3.0)
	CHECKF(sqrti_f, 81.0, 9.0)
	CHECKD(negi_d, -2.0, 2.0)
	CHECKD(absi_d, -1.0, 1.0)
	CHECKD(sqrti_d, 9.0, 3.0)

	prepare
		pushargi ok
	finishi @puts
	ret
	epilog
