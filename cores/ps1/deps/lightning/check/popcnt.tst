.data	4096
pop_tab:
.c	0 1 1 2 1 2 2 3 1 2 2 3 2 3 3 4 1 2 2 3 2 3 3 4 2 3 3 4 3 4 4 5 1 2 2 3 2 3 3 4 2 3 3 4 3 4 4 5 2 3 3 4 3 4 4 5 3 4 4 5 4 5 5 6 1 2 2 3 2 3 3 4 2 3 3 4 3 4 4 5 2 3 3 4 3 4 4 5 3 4 4 5 4 5 5 6 2 3 3 4 3 4 4 5 3 4 4 5 4 5 5 6 3 4 4 5 4 5 5 6 4 5 5 6 5 6 6 7 1 2 2 3 2 3 3 4 2 3 3 4 3 4 4 5 2 3 3 4 3 4 4 5 3 4 4 5 4 5 5 6 2 3 3 4 3 4 4 5 3 4 4 5 4 5 5 6 3 4 4 5 4 5 5 6 4 5 5 6 5 6 6 7 2 3 3 4 3 4 4 5 3 4 4 5 4 5 5 6 3 4 4 5 4 5 5 6 4 5 5 6 5 6 6 7 3 4 4 5 4 5 5 6 4 5 5 6 5 6 6 7 4 5 5 6 5 6 6 7 5 6 6 7 6 7 7 8
ok:
.c	"ok\n"
fmt:
#if __WORDSIZE == 32
.c	"0x%08lx = %d\n"
#else
.c	"0x%016lx = %d\n"
#endif

#define BIT2(OP, ARG, RES, R0, R1)			\
	movi %R1 ARG					\
	OP##r %R0 %R1					\
	beqi OP##R0##R1##ARG %R0 RES			\
	calli @abort					\
OP##R0##R1##ARG:

#define BIT1(OP, ARG, RES, V0, V1, V2, R0, R1, R2)	\
	BIT2(OP, ARG, RES, V0, V0)			\
	BIT2(OP, ARG, RES, V0, V1)			\
	BIT2(OP, ARG, RES, V0, V2)			\
	BIT2(OP, ARG, RES, V0, R0)			\
	BIT2(OP, ARG, RES, V0, R1)			\
	BIT2(OP, ARG, RES, V0, R2)

#define  BIT(OP, ARG, RES, V0, V1, V2, R0, R1, R2)	\
	BIT1(OP, ARG, RES, V1, V2, R0, R1, R2, V0)	\
	BIT1(OP, ARG, RES, V2, R0, R1, R2, V0, V1)	\
	BIT1(OP, ARG, RES, R0, R1, R2, V0, V1, V2)	\
	BIT1(OP, ARG, RES, R1, R2, V0, V1, V2, R0)	\
	BIT1(OP, ARG, RES, R2, V0, V1, V2, R0, R1)

#define POPCNT(ARG, RES)				\
	BIT(popcnt, ARG, RES, v0, v1, v2, r0, r1, r2)

.code
	jmpi main
name popcnt
popcnt:
	prolog
	arg $in
	getarg %r1 $in
	extr_uc %r2 %r1
	movi %v0 pop_tab
	ldxr_uc %r0 %v0 %r2
	movi %v1 8
popcnt_loop:
	rshr %r2 %r1 %v1
	extr_uc %r2 %r2
	ldxr_uc %r2 %v0 %r2
	addr %r0 %r0 %r2
	addi %v1 %v1 8
	blti popcnt_loop %v1 __WORDSIZE
	retr %r0
	epilog

#if 0
	name main
main:
	prolog
	arg $argc
	arg $argv
	getarg %r0 $argc
	bnei default %r0 2
	getarg %v0 $argv
	ldxi %r0 %v0 $(__WORDSIZE >> 3)
	prepare
		pushargr %r0
		pushargi 0
		pushargi 0
	finishi @strtoul
	retval %v0
	jmpi main_do
default:
#if __WORDSIZE == 32
    movi %v0 0x8a13c851
#else
    movi %v0 0x984a137ffec85219
#endif
main_do:
	prepare
		pushargr %v0
	finishi popcnt
	retval %r0
	prepare
		pushargi fmt
		ellipsis
		pushargr %v0
		pushargr %r0
	finishi @printf

	popcntr %r0 %v0
	prepare
		pushargi fmt
		ellipsis
		pushargr %v0
		pushargr %r0
	finishi @printf

	ret
	epilog
#else

	name main
main:
	prolog
#if __WORDSIZE == 32
	POPCNT(0x8a13c851, 12)
	POPCNT(0x12345678, 13)
	POPCNT(0x02468ace, 12)
#else
	POPCNT(0x984a137ffec85219, 32)
	POPCNT(0x123456789abcdef0, 32)
	POPCNT(0x02468ace013579bd, 28)
#endif
	prepare
		pushargi ok
	finishi @printf
	reti 0
	epilog
#endif
