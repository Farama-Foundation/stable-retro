.data	4096
swap_tab:
.c	0 128 64 192 32 160  96 224 16 144 80 208 48 176 112 240 8 136 72 200 40 168 104 232 24 152 88 216 56 184 120 248 4 132 68 196 36 164 100 228 20 148 84 212 52 180 116 244 12 140 76 204 44 172 108 236 28 156 92 220 60 188 124 252  2 130 66 194 34 162  98 226 18 146 82 210 50 178 114 242 10 138 74 202 42 170 106 234 26 154 90 218 58 186 122 250 6 134 70 198 38 166 102 230 22 150 86 214 54 182 118 246 14 142 78 206 46 174 110 238 30 158 94 222 62 190 126 254 1 129 65 193 33 161  97 225 17 145 81 209 49 177 113 241  9 137 73 201 41 169 105 233 25 153 89 217 57 185 121 249 5 133 69 197 37 165 101 229 21 149 85 213 53 181 117 245 13 141 77 205 45 173 109 237 29 157 93 221 61 189 125 253 3 131 67 195 35 163  99 227 19 147 83 211 51 179 115 243 11 139 75 203 43 171 107 235 27 155 91 219 59 187 123 251 7 135 71 199 39 167 103 231 23 151 87 215 55 183 119 247 15 143 79 207 47 175 111 239 31 159 95 223 63 191 127 255
ok:
.c	"ok\n"
fmt:
#if __WORDSIZE == 32
.c	"0x%08lx = 0x%08lx\n"
#else
.c	"0x%016lx = 0x%016lx\n"
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

#define RBIT(ARG, RES)					\
	BIT(rbit, ARG, RES, v0, v1, v2, r0, r1, r2)

.code
	jmpi main
	name rbit_table
rbit_table:
	prolog
	arg $in
	getarg %r1 $in
	extr_uc %r2 %r1
	movi %v0 swap_tab
	ldxr_uc %r0 %v0 %r2
	movi %v1 8
rbit_table_loop:
	rshr %r2 %r1 %v1
	extr_uc %r2 %r2
	lshi %r0 %r0 8
	ldxr_uc %r2 %v0 %r2
	orr %r0 %r0 %r2
	addi %v1 %v1 8
	blti rbit_table_loop %v1 __WORDSIZE
	retr %r0
	epilog

	name rbit_unrolled
rbit_unrolled:
	prolog
	arg $in
	getarg %r0 $in
#if __WORDSIZE == 32
	movi %r1 0x55555555
#else
	movi %r1 0x5555555555555555
#endif
	rshi_u %r2 %r0 1	// r2 = r0 >> 1
	andr %r2 %r2 %r1	// r2 &= r1
	andr %v0 %r0 %r1	// v0 = r0 & r1
	lshi %v0 %v0 1		// v0 <<= 1
	orr %r0 %r2 %v0		// r0 = r2 | v0
#if __WORDSIZE == 32
	movi %r1 0x33333333
#else
	movi %r1 0x3333333333333333
#endif
	rshi_u %r2 %r0 2	// r2 = r0 >> 2
	andr %r2 %r2 %r1	// r2 &= r1
	andr %v0 %r0 %r1	// v0 = r0 & r1
	lshi %v0 %v0 2		// v0 <<= 2
	orr %r0 %r2 %v0		// r0 = r2 | v0
#if __WORDSIZE == 32
	movi %r1 0x0f0f0f0f
#else
	movi %r1 0x0f0f0f0f0f0f0f0f
#endif
	rshi_u %r2 %r0 4	// r2 = r0 >> 4
	andr %r2 %r2 %r1	// r2 &= r1
	andr %v0 %r0 %r1	// v0 = r0 & r1
	lshi %v0 %v0 4		// v0 <<= 4
	orr %r0 %r2 %v0		// r0 = r2 | v0
#if __WORDSIZE == 32
	movi %r1 0x00ff00ff
#else
	movi %r1 0x00ff00ff00ff00ff
#endif
	rshi_u %r2 %r0 8	// r2 = r0 >> 8
	andr %r2 %r2 %r1	// r2 &= r1
	andr %v0 %r0 %r1	// v0 = r0 & r1
	lshi %v0 %v0 8		// v0 <<= 8
	orr %r0 %r2 %v0		// r0 = r2 | v0
#if __WORDSIZE == 32
	rshi_u %r2 %r0 16	// r2 = r0 >> 16
	lshi %v0 %r0 16		// v0 = r0 << 16
	orr %r0 %r2 %v0		// r0 = r2 | v0
#else
	movi %r1 0x0000ffff0000ffff
	rshi_u %r2 %r0 16	// r2 = r0 >> 16
	andr %r2 %r2 %r1	// r2 &= r1
	andr %v0 %r0 %r1	// v0 = r0 & r1
	lshi %v0 %v0 16		// v0 <<= 16
	orr %r0 %r2 %v0		// r0 = r2 | v0
	rshi_u %r2 %r0 32	// r2 = r0 >> 32
	lshi %v0 %r0 32		// v0 = r0 << 32
	orr %r0 %r2 %v0		// r0 = r2 | v0
#endif
	retr %r0
	epilog

	name rbit_loop
rbit_loop:
	prolog
	arg $in
	getarg %r0 $in
	movi %r1 __WORDSIZE
	movi %r2 $(~0)
rbit_loop_loop:			  // while (%r1 >>= 1) > 0
	rshi %r1 %r1 1		  //	    %r1 >>= 1
	blei rbit_loop_done %r1 0 // no loop if %r1 <= 0
	lshr %v0 %r2 %r1	  // %v0 = %r2 << %r1
	xorr %r2 %r2 %v0	  // %r2 ^= %v0
	rshr %v0 %r0 %r1	  // %v0 = %r0 >> %r1
	andr %v0 %v0 %r2	  // %r2 = %v0 & %r2
	lshr %v1 %r0 %r1	  // %v1 = %r0 << %r1
	comr %r0 %r2		  // %r0 = ~%r2
	andr %v1 %r0 %v1	  // %v1 &= %r0
	orr %r0 %v0 %v1		  // %r0 = %v0 | %v1
	jmpi rbit_loop_loop
rbit_loop_done:
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
	finishi rbit_table
	retval %r0
	prepare
		pushargi fmt
		ellipsis
		pushargr %v0
		pushargr %r0
	finishi @printf

	prepare
		pushargr %v0
	finishi rbit_unrolled
	retval %r0
	prepare
		pushargi fmt
		ellipsis
		pushargr %v0
		pushargr %r0
	finishi @printf

	prepare
		pushargr %v0
	finishi rbit_loop
	retval %r0
	prepare
		pushargi fmt
		ellipsis
		pushargr %v0
		pushargr %r0
	finishi @printf

	rbitr %r0 %v0
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
	RBIT(0x8a13c851, 0x8a13c851)
	RBIT(0x12345678, 0x1e6a2c48)
	RBIT(0x02468ace, 0x73516240)
#else
	RBIT(0x984a137ffec85219, 0x984a137ffec85219)
	RBIT(0x123456789abcdef0, 0x0f7b3d591e6a2c48)
	RBIT(0x02468ace013579bd, 0xbd9eac8073516240)
#endif
	prepare
		pushargi ok
	finishi @printf
	reti 0
	epilog
#endif
