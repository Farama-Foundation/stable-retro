#include "alu.inc"

.code
	prolog

#define LROT(N, I0, I1, V)	ALU(N, , lrot, I0, I1, V)
	LROT(0,	0x7f,			1,	0xfe)
#if __WORDSIZE == 32
	LROT(1,	0xfffffffe,		31,	0x7fffffff)
	LROT(2,	0x12345678,		11,	0xa2b3c091)
	LROT(3,	0x80000001,		1,	0x03)
#else
	LROT(1,	0xfffffffffffffffe,	31,	0xffffffff7fffffff)
	LROT(2,	0x123456789abcdef0,	43,	0xe6f78091a2b3c4d5)
	LROT(3,	0x00000001ffffffff,	32,	0xffffffff00000001)
	LROT(4,	0x80000001,		33,	0x200000001)
	LROT(5,	0x8000000000,		35,	0x400)
#endif

#define RROT(N, I0, I1, V)	ALU(N, , rrot, I0, I1, V)
	RROT(0,	0xfe,			1,	0x7f)
#if __WORDSIZE == 32
	RROT(1,	0xfffffffe,		31,	0xfffffffd)
	RROT(2,	0x12345678,		11,	0xcf02468a)
	RROT(3,	0x80000001,		3,	0x30000000)
#else
	RROT(1,	0xfffffffffffffffe,	31,	0xfffffffdffffffff)
	RROT(2,	0x123456789abcdef0,	43,	0xcf13579bde02468a)
	RROT(3,	0x00000001ffffffff,	32,	0xffffffff00000001)
	RROT(4,	0x80000001,		33,	0x4000000080000000)
	RROT(5,	0x8000000000,		35,	0x10)
#endif

	prepare
		pushargi ok
		ellipsis
	finishi @printf
	ret
	epilog
