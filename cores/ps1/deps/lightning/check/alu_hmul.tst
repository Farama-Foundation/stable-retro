#include "alu.inc"

.code
	prolog
#define HMUL(N, I0, I1, V)		ALU(N, , hmul, I0, I1, V)
#define UHMUL(N, I0, I1, V)		ALU(N, _u, hmul, I0, I1, V)
	HMUL(0, -2, -1, 0)
	HMUL(1, 0, -1, 0)
	HMUL(2, -1, 0, 0)
	HMUL(3, 1, -1, -1)
#if __WORDSIZE == 32
	 HMUL(4, 0x7ffff, 0x7ffff, 0x3f)
	UHMUL(5, 0xffffff, 0xffffff, 0xffff)
	 HMUL(6, 0x80000000, -2, 1)
	 HMUL(7, 0x80000000, 2, -1)
	 HMUL(8, 0x80000001, 3, -2)
	 HMUL(9, 0x80000001, -3, 1)
#else
	 HMUL(4, 0x7ffffffff, 0x7ffffffff, 0x3f)
	UHMUL(5, 0xffffffffff, 0xffffffffff, 0xffff)
	 HMUL(6, 0x8000000000000000, -2, 1)
	 HMUL(7, 0x8000000000000000, 2, -1)
	 HMUL(8, 0x8000000000000001, 3, -2)
	 HMUL(9, 0x8000000000000001, -3, 1)
#endif
	prepare
		pushargi ok
		ellipsis
	finishi @printf
	ret
	epilog
