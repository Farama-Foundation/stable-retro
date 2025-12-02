.data	12
ok:
.c	"ok\n"

#if __WORDSIZE == 32
#  define w7f		0x7fffffff
#  define w80		0x80000000
#  define w81		0x80000001
#  define wff		0xffffffff
#  define LEXTII2(N, R0, R1)
#  define LEXTIC2(N, R0)
#else
#  define w7f		0x7fffffffffffffff
#  define w80		0x8000000000000000
#  define w81		0x8000000000000001
#  define wff		0xffffffffffffffff
#  define i7f		wff
#  define ui7f		0xffffffff
#  define i80		0
#  define ui80		0
#  define i81		1
#  define ui81		1
#  define iff		wff
#  define uiff		0xffffffff
#  define LEXTII2(N, R0, R1)					\
	EXTII2(N, i, R0, R1)					\
	EXTII2(N, ui, R0, R1)
#  define LEXTIC2(N, R0)					\
	EXTIC2(N, i, R0)					\
	EXTIC2(N, ui, R0)
#endif
#define c7f		wff
#define uc7f		0xff
#define s7f		wff
#define us7f		0xffff
#define c80		0
#define uc80		0
#define s80		0
#define us80		0
#define c81		1
#define uc81		1
#define s81		1
#define us81		1
#define cff		wff
#define ucff		0xff
#define sff		wff
#define usff		0xffff

#define EXTII2(N, T, R0, R1)					\
	movi %R0 w##N						\
	extr_##T %R1 %R0					\
	beqi T##_##R0##_##R1##_##N %R1 T##N			\
	calli @abort						\
T##_##R0##_##R1##_##N:
#define EXTII1(N, R0, R1)					\
	EXTII2(N, c, R0, R1)					\
	EXTII2(N, uc, R0, R1)					\
	EXTII2(N, s, R0, R1)					\
	EXTII2(N, us, R0, R1)					\
	LEXTII2(N, R0, R1)
#define EXTII0(R0, R1)						\
	EXTII1(7f, R0, R1)					\
	EXTII1(80, R0, R1)					\
	EXTII1(81, R0, R1)					\
	EXTII1(ff, R0, R1)

#define EXTIC2(N, T, R0)					\
	movi %R0 w##N						\
	extr_##T %R0 %R0					\
	beqi T##_##R0##_##N %R0 T##N				\
	calli @abort						\
T##_##R0##_##N:
#define EXTIC1(N, R0)						\
	EXTIC2(N, c, R0)					\
	EXTIC2(N, uc, R0)					\
	EXTIC2(N, s, R0)					\
	EXTIC2(N, us, R0)					\
	LEXTIC2(N, R0)
#define EXTIC0(R0)						\
	EXTIC1(7f, R0)						\
	EXTIC1(80, R0)						\
	EXTIC1(81, R0)						\
	EXTIC1(ff, R0)

#define EXTII(V0, V1, V2, R0, R1, R2)				\
	EXTII0(V0, V1)						\
	EXTII0(V0, V2)						\
	EXTII0(V0, R0)						\
	EXTII0(V0, R1)						\
	EXTII0(V0, R2)						\
	EXTII0(V1, V0)						\
	EXTII0(V1, V2)						\
	EXTII0(V1, R0)						\
	EXTII0(V1, R1)						\
	EXTII0(V1, R2)						\
	EXTII0(V2, V0)						\
	EXTII0(V2, V1)						\
	EXTII0(V2, R0)						\
	EXTII0(V2, R1)						\
	EXTII0(V2, R2)						\
	EXTII0(R0, V0)						\
	EXTII0(R0, V1)						\
	EXTII0(R0, V2)						\
	EXTII0(R0, R1)						\
	EXTII0(R0, R2)						\
	EXTII0(R1, V0)						\
	EXTII0(R1, V1)						\
	EXTII0(R1, V2)						\
	EXTII0(R1, R0)						\
	EXTII0(R1, R2)						\
	EXTII0(R2, V0)						\
	EXTII0(R2, V1)						\
	EXTII0(R2, V2)						\
	EXTII0(R2, R0)						\
	EXTII0(R2, R1)						\
	EXTIC0(V0)						\
	EXTIC0(V1)						\
	EXTIC0(V2)						\
	EXTIC0(R0)						\
	EXTIC0(R1)						\
	EXTIC0(R2)

#define EXIF1(N, V, R0, R1)					\
	movi %R0 V						\
	extr_f %R1 %R0						\
	beqi_f wf##_##R0##_##R1##_##N %R1 V			\
wf##_##R0##_##R1##_##N:
#define EXIF0(R0, R1)						\
	EXIF1(0, -1, R0, R1)					\
	EXIF1(1, 64, R0, R1)
#define EXIF(V0, V1, V2, R0, R1, R2, F0, F1, F2, F3, F4, F5)	\
	EXIF0(V0, F0)						\
	EXIF0(V1, F1)						\
	EXIF0(V2, F2)						\
	EXIF0(R0, F3)						\
	EXIF0(R1, F4)						\
	EXIF0(R2, F5)
#define EXID1(N, V, R0, R1)					\
	movi %R0 V						\
	extr_d %R1 %R0						\
	beqi_d wd##_##R0##_##R1##_##N %R1 V			\
wd##_##R0##_##R1##_##N:
#define EXID0(R0, R1)						\
	EXID1(0, -1, R0, R1)					\
	EXID1(1, 64, R0, R1)
#define EXID(V0, V1, V2, R0, R1, R2, F0, F1, F2, F3, F4, F5)	\
	EXID0(V0, F0)						\
	EXID0(V1, F1)						\
	EXID0(V2, F2)						\
	EXID0(R0, F3)						\
	EXID0(R1, F4)						\
	EXID0(R2, F5)

#define EXFI1(N, V, R0, R1)					\
	movi_f %R1 V						\
	truncr_f %R0 %R1					\
	beqi fi##_##R0##_##R1##_##N %R0 V			\
	calli @abort						\
fi##_##R0##_##R1##_##N:
#define EXFI0(R0, R1)						\
	EXFI1(0,   42, R0, R1)					\
	EXFI1(1, -128, R0, R1)
#define EXFI(V0, V1, V2, R0, R1, R2, F0, F1, F2, F3, F4, F5)	\
	EXFI0(V0, F5)						\
	EXFI0(V1, F4)						\
	EXFI0(V2, F3)						\
	EXFI0(R0, F2)						\
	EXFI0(R1, F1)						\
	EXFI0(R2, F0)
#define EXDI1(N, V, R0, R1)					\
	movi_d %R1 V						\
	truncr_d %R0 %R1					\
	beqi di##_##R0##_##R1##_##N %R0 V			\
	calli @abort						\
di##_##R0##_##R1##_##N:
#define EXDI0(R0, R1)						\
	EXDI1(0,   42, R0, R1)					\
	EXDI1(1, -128, R0, R1)
#define EXDI(V0, V1, V2, R0, R1, R2, F0, F1, F2, F3, F4, F5)	\
	EXDI0(V0, F5)						\
	EXDI0(V1, F4)						\
	EXDI0(V2, F3)						\
	EXDI0(R0, F2)						\
	EXDI0(R1, F1)						\
	EXDI0(R2, F0)

#define LEXFI1(N, V, R0, R1)					\
	movi_f %R1 V						\
	truncr_f_i %R0 %R1					\
	andi %R0 %R0 0xffffffff					\
	beqi lfi##_##R0##_##R1##_##N %R0 $(V & 0xffffffff)	\
	calli @abort						\
lfi##_##R0##_##R1##_##N:
#define LEXFI0(R0, R1)						\
	LEXFI1(0,   42, R0, R1)					\
	LEXFI1(1, -128, R0, R1)
#define LEXFI(V0, V1, V2, R0, R1, R2, F0, F1, F2, F3, F4, F5)	\
	LEXFI0(V0, F5)						\
	LEXFI0(V1, F4)						\
	LEXFI0(V2, F3)						\
	LEXFI0(R0, F2)						\
	LEXFI0(R1, F1)						\
	LEXFI0(R2, F0)
#define LEXDI1(N, V, R0, R1)					\
	movi_d %R1 V						\
	truncr_d_i %R0 %R1					\
	andi %R0 %R0 0xffffffff					\
	beqi ldi##_##R0##_##R1##_##N %R0 $(V & 0xffffffff)	\
	calli @abort						\
ldi##_##R0##_##R1##_##N:
#define LEXDI0(R0, R1)						\
	LEXDI1(0,   42, R0, R1)					\
	LEXDI1(1, -128, R0, R1)
#define LEXDI(V0, V1, V2, R0, R1, R2, F0, F1, F2, F3, F4, F5)	\
	LEXDI0(V0, F5)						\
	LEXDI0(V1, F4)						\
	LEXDI0(V2, F3)						\
	LEXDI0(R0, F2)						\
	LEXDI0(R1, F1)						\
	LEXDI0(R2, F0)

#define EXTFD2(V, R0, R1)					\
	movi_f %R0 V						\
	extr_f_d %R1 %R0					\
	beqi_d fd##_##R0##_##R1 %R1 V				\
	calli @abort						\
fd##_##R0##_##R1:
#define EXTFD1(R0, R1)						\
	EXTFD2(1.25, R0, R1)
#define EXTFDC2(V, R0)						\
	movi_f %R0 V						\
	extr_f_d %R0 %R0					\
	beqi_d fdc##_##R0 %R0 V					\
	calli @abort						\
fdc##_##R0:
#define EXTFDC1(R0)						\
	EXTFDC2(-0.75, R0)
#define EXTFD(R0, R1, R2, R3, R4, R5)				\
	EXTFD1(R0, R1)						\
	EXTFD1(R0, R2)						\
	EXTFD1(R0, R3)						\
	EXTFD1(R0, R4)						\
	EXTFD1(R0, R5)						\
	EXTFDC1(R0)						\
	EXTFDC1(R1)						\
	EXTFDC1(R2)						\
	EXTFDC1(R3)						\
	EXTFDC1(R4)						\
	EXTFDC1(R5)

#define EXTDF2(V, R0, R1)					\
	movi_d %R0 V						\
	extr_d_f %R1 %R0					\
	beqi_f df##_##R0##_##R1 %R1 V				\
	calli @abort						\
df##_##R0##_##R1:
#define EXTDF1(R0, R1)						\
	EXTDF2(1.25, R0, R1)
#define EXTDFC2(V, R0)						\
	movi_d %R0 V						\
	extr_d_f %R0 %R0					\
	beqi_f dfc##_##R0 %R0 V					\
	calli @abort						\
dfc##_##R0:
#define EXTDFC1(R0)						\
	EXTDFC2(-0.75, R0)
#define EXTDF(R0, R1, R2, R3, R4, R5)				\
	EXTDF1(R0, R1)						\
	EXTDF1(R0, R2)						\
	EXTDF1(R0, R3)						\
	EXTDF1(R0, R4)						\
	EXTDF1(R0, R5)						\
	EXTDFC1(R0)						\
	EXTDFC1(R1)						\
	EXTDFC1(R2)						\
	EXTDFC1(R3)						\
	EXTDFC1(R4)						\
	EXTDFC1(R5)

#define MVFW(R0, F0, F1)					\
	movi_f %F0 0.5						\
	movr_f_w %R0 %F0					\
	movr_w_f %F1 %R0					\
	beqr_f fw##R0##F0##F1 %F0 %F1				\
	calli @abort						\
fw##R0##F0##F1:
#define FW(R0, R1, R2, R3, R4, R5, F0, F1, F2, F3, F4, F5)	\
	MVFW(R0, F0, F1)					\
	MVFW(R1, F1, F2)					\
	MVFW(R2, F2, F3)					\
	MVFW(R3, F3, F4)					\
	MVFW(R4, F4, F5)					\
	MVFW(R5, F5, F0)
#define MVFWI(R0, F0, F1)					\
	movi_f %F1 0.25						\
	movi_f_w %R0 0.25					\
	movr_w_f %F0 %R0					\
	beqr_f ifw##R0##F0##F1 %F0 %F1				\
	calli @abort						\
ifw##R0##F0##F1:
#define FWI(R0, R1, R2, R3, R4, R5, F0, F1, F2, F3, F4, F5)	\
	MVFWI(R0, F0, F1)					\
	MVFWI(R1, F1, F2)					\
	MVFWI(R2, F2, F3)					\
	MVFWI(R3, F3, F4)					\
	MVFWI(R4, F4, F5)					\
	MVFWI(R5, F5, F0)

#if __WORDSIZE == 32
#  define MVDW(R0, R1, F0, F1)					\
	movi_d %F0 0.5						\
	movr_d_ww %R0 %R1 %F0					\
	movr_ww_d %F1 %R0 %R1					\
	beqr_d dw##R0##R1##F0##F1 %F0 %F1			\
	calli @abort						\
dw##R0##R1##F0##F1:
#  define DW(R0, R1, R2, R3, R4, R5, F0, F1, F2, F3, F4, F5)	\
	MVDW(R0, R1, F0, F1)					\
	MVDW(R1, R2, F1, F2)					\
	MVDW(R2, R3, F2, F3)					\
	MVDW(R3, R4, F3, F4)					\
	MVDW(R4, R5, F4, F5)					\
	MVDW(R5, R0, F5, F0)
#  define MVDWI(R0, R1, F0, F1)					\
	movi_d %F1 0.25						\
	movi_d_ww %R0 %R1 0.25					\
	movr_ww_d %F0 %R0 %R1					\
	beqr_d idw##R0##R1##F0##F1 %F0 %F1			\
	calli @abort						\
idw##R0##R1##F0##F1:
#  define DWI(R0, R1, R2, R3, R4, R5, F0, F1, F2, F3, F4, F5)	\
	MVDWI(R0, R1, F0, F1)					\
	MVDWI(R1, R2, F1, F2)					\
	MVDWI(R2, R3, F2, F3)					\
	MVDWI(R3, R4, F3, F4)					\
	MVDWI(R4, R5, F4, F5)					\
	MVDWI(R5, R0, F5, F0)
#else
#  define MVDW(R0, F0, F1)					\
	movi_d %F0 0.5						\
	movr_d_w %R0 %F0					\
	movr_w_d %F1 %R0					\
	beqr_d dw##R0##F0##F1 %F0 %F1				\
	calli @abort						\
dw##R0##F0##F1:
#  define DW(R0, R1, R2, R3, R4, R5, F0, F1, F2, F3, F4, F5)	\
	MVDW(R0, F0, F1)					\
	MVDW(R1, F1, F2)					\
	MVDW(R2, F2, F3)					\
	MVDW(R3, F3, F4)					\
	MVDW(R4, F4, F5)					\
	MVDW(R5, F5, F0)
#  define MVDWI(R0, F0, F1)					\
	movi_d %F1 0.25						\
	movi_d_w %R0 0.25					\
	movr_w_d %F0 %R0					\
	beqr_d idw##R0##F0##F1 %F0 %F1				\
	calli @abort						\
idw##R0##F0##F1:
#  define DWI(R0, R1, R2, R3, R4, R5, F0, F1, F2, F3, F4, F5)	\
	MVDWI(R0, F0, F1)					\
	MVDWI(R1, F1, F2)					\
	MVDWI(R2, F2, F3)					\
	MVDWI(R3, F3, F4)					\
	MVDWI(R4, F4, F5)					\
	MVDWI(R5, F5, F0)
#endif

#define MVWFI(F0, F1)						\
	movi_f %F1 0.25						\
	movi_w_f %F0 0x3e800000					\
	beqr_f iwf##F0##F1 %F0 %F1				\
	calli @abort						\
iwf##F0##F1:
#define WFI(F0, F1, F2, F3, F4, F5)				\
	MVWFI(F0, F1)						\
	MVWFI(F1, F2)						\
	MVWFI(F2, F3)						\
	MVWFI(F3, F4)						\
	MVWFI(F4, F5)						\
	MVWFI(F5, F0)

#if __WORDSIZE == 32
#    define MVWDI(F0, F1)					\
	movi_d %F1 0.5						\
	movi_ww_d %F0 0 0x3fe00000				\
	beqr_d iwwd##F0##F1 %F0 %F1				\
	calli @abort						\
iwwd##F0##F1:
#    define WDI(F0, F1, F2, F3, F4, F5)				\
	MVWDI(F0, F1)						\
	MVWDI(F1, F2)						\
	MVWDI(F2, F3)						\
	MVWDI(F3, F4)						\
	MVWDI(F4, F5)						\
	MVWDI(F5, F0)
#else
#  define MVWDI(F0, F1)						\
	movi_d %F1 0.5						\
	movi_w_d %F0 0x3fe0000000000000				\
	beqr_d iwd##F0##F1 %F0 %F1				\
	calli @abort						\
iwd##F0##F1:
#define WDI(F0, F1, F2, F3, F4, F5)				\
	MVWDI(F0, F1)						\
	MVWDI(F1, F2)						\
	MVWDI(F2, F3)						\
	MVWDI(F3, F4)						\
	MVWDI(F4, F5)						\
	MVWDI(F5, F0)
#endif

.code
	prolog

	/* simple sequence for easier disassembly reading and encoding check */
	movi %r0 w7f
	extr_c %r1 %r0
	beqi xc %r1 c7f
	calli @abort
xc:
	movi %r0 w7f
	extr_uc %r1 %r0
	beqi xuc %r1 uc7f
	calli @abort
xuc:
	movi %r0 w7f
	extr_s %r1 %r0
	beqi xs %r1 s7f
	calli @abort
xs:
	movi %r0 w7f
	extr_us %r1 %r0
	beqi xus %r1 us7f
	calli @abort
xus:
#if __WORDSIZE == 64
	movi %r0 w7f
	extr_i %r1 %r0
	beqi xi %r1 i7f
	calli @abort
xi:
	movi %r0 w7f
	extr_ui %r1 %r0
	beqi xui %r1 ui7f
	calli @abort
xui:
#endif
	movi %r0 -2
	extr_f %f0 %r0
	beqi_f xif %f0 -2
	calli @abort
xif:
	movi %r0 32
	extr_d %f0 %r0
	beqi_d xid %f0 32
	calli @abort
xid:
	movi_f %f0 -128
	truncr_f %r0 %f0
	beqi xfi %r0 -128
	calli @abort
xfi:
	movi_d %f0 -128
	truncr_d %r0 %f0
	beqi xdi %r0 -128
	calli @abort
xdi:
#if __WORDSIZE == 64
	movi_f %f0 -128
	truncr_f_i %r0 %f0
	andi %r0 %r0 0xffffffff
	beqi yfi %r0 $(-128 & 0xffffffff)
	calli @abort
yfi:
	movi_d %f0 -128
	truncr_d_i %r0 %f0
	andi %r0 %r0 0xffffffff
	beqi ydi %r0 $(-128 & 0xffffffff)
	calli @abort
ydi:
#endif
	movi_f %f0 0.5
	extr_f_d %f1 %f0
	beqi_d xfd %f1 0.5
	calli @abort
xfd:
	movi_d %f0 0.5
	extr_d_f %f1 %f0
	beqi_f xdf %f1 0.5
	calli @abort
xdf:
	movi_f %f0 0.5
	movr_f_w %r0 %f0
	movr_w_f %f1 %r0
	beqr_f fw %f0 %f1
	calli @abort
fw:
	movi_f %f1 0.25
	movi_f_w %r0 0.25
	movr_w_f %f0 %r0
	beqr_f ifw %f0 %f1
	calli @abort
ifw:
#if __WORDSIZE == 32
	movi_d %f0 0.5
	movr_d_ww %r0 %r1 %f0
	movr_ww_d %f1 %r0 %r1
	beqr_d dww %f0 %f1
	calli @abort
dww:
	movi_d %f1 0.25
	movi_d_ww %r0 %r1 0.25
	movr_ww_d %f0 %r0 %r1
	beqr_d idww %f0 %f1
	calli @abort
idww:
#else
	movi_d %f0 0.5
	movr_d_w %r0 %f0
	movr_w_d %f1 %r0
	beqr_d dw %f0 %f1
	calli @abort
dw:
	movi_d %f1 0.25
	movi_d_w %r0 0.25
	movr_w_d %f0 %r0
	beqr_d idw %f0 %f1
	calli @abort
idw:
#endif

	movi_f %f1 0.25
	movi_w_f %f0 0x3e800000
	beqr_f iwf %f0 %f1
	calli @abort
iwf:
#if __WORDSIZE == 32
	movi_d %f1 0.5
	movi_ww_d %f0 0 0x3fe00000
	beqr_d iwwd %f0 %f1
	calli @abort
iwwd:
#else
	movi_d %f1 0.5
	movi_w_d %f0 0x3fe0000000000000
	beqr_d iwd %f0 %f1
	calli @abort
iwd:
#endif

	EXTII(v0, v1, v2, r0, r1, r2)
	EXIF(v0, v1, v2, r0, r1, r2, f0, f1, f2, f3, f4, f5)
	EXID(v0, v1, v2, r0, r1, r2, f0, f1, f2, f3, f4, f5)
	EXFI(v0, v1, v2, r0, r1, r2, f0, f1, f2, f3, f4, f5)
#if __WORDSIZE == 64
	LEXFI(v0, v1, v2, r0, r1, r2, f0, f1, f2, f3, f4, f5)
	LEXDI(v0, v1, v2, r0, r1, r2, f0, f1, f2, f3, f4, f5)
#endif
	EXDI(v0, v1, v2, r0, r1, r2, f0, f1, f2, f3, f4, f5)
	EXTFD(f0, f1, f2, f3, f4, f5)
	EXTDF(f0, f1, f2, f3, f4, f5)
	FW(v0, v1, v2, r0, r1, r2, f0, f1, f2, f3, f4, f5)
	FWI(v0, v1, v2, r0, r1, r2, f0, f1, f2, f3, f4, f5)
	DW(v0, v1, v2, r0, r1, r2, f0, f1, f2, f3, f4, f5)
	DWI(v0, v1, v2, r0, r1, r2, f0, f1, f2, f3, f4, f5)
	WFI(f0, f1, f2, f3, f4, f5)
	WDI(f0, f1, f2, f3, f4, f5)

	// just to know did not abort
	prepare
		pushargi ok
		ellipsis
	finishi @printf

	ret
	epilog
