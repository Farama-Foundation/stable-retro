#include <stdio.h>

/*   Test that jit_extr(), jit_extr_u() and jit_depr() work correctly with
 * C bitfields.
 *   Note that to be compatible with C bitfields, behavior is different
 * for big endian and little endian. For big endian, the bitfield
 * offset is changed to (__WORDSIZE - offset).
 *   The logic is to create a C function that receives a pointer to
 * a single word union (that has a bitfield and a word) and a jit function
 * pointer that knows about the bitfield. The C function calls the jit
 * function passing the pointer to the bitfield, and the jit function
 * changes the bitfield with a computed known value specific for the test.
 *   After calling the jit function, the C function validates that the
 * bitfield has the expected value, and also validates no bits outside of
 * the bitfield were modified.
 *   The test is done for signed and unsigned bitfields.
 *   The test does a brute force validation that all possible bitfields
 * using a machine word work as expected.
 *   Tests for possible register clobber is done in check/ext.tst.
 *   The test is dynamically generated because the generated source file
 * is too large to be kept under version control.
 */

int
main(int argc, char *argv[])
{
    int			i, j;
    puts("\
#include <stdio.h>\n\
#include <lightning.h>\n\
\n\
#  if __WORDSIZE == 64 && _WIN32\n\
#    define ONE				1LL\n\
#  else\n\
#    define ONE				1L\n\
#  endif\n\
/* Avoid clang running out of memory on OpenBSD mips64 */\n\
#define SKIP_64_BITS	(__mips__ && __clang__)\n\
\n\
#define GENMASK1()		GENMASK(1)\n\
#define GENMASK2()		GENMASK(2)\n\
#define GENMASK3()		GENMASK(3)\n\
#define GENMASK4()		GENMASK(4)\n\
#define GENMASK5()		GENMASK(5)\n\
#define GENMASK6()		GENMASK(6)\n\
#define GENMASK7()		GENMASK(7)\n\
#define GENMASK8()		GENMASK(8)\n\
#define GENMASK9()		GENMASK(9)\n\
#define GENMASK10()		GENMASK(10)\n\
#define GENMASK11()		GENMASK(11)\n\
#define GENMASK12()		GENMASK(12)\n\
#define GENMASK13()		GENMASK(13)\n\
#define GENMASK14()		GENMASK(14)\n\
#define GENMASK15()		GENMASK(15)\n\
#define GENMASK16()		GENMASK(16)\n\
#define GENMASK17()		GENMASK(17)\n\
#define GENMASK18()		GENMASK(18)\n\
#define GENMASK19()		GENMASK(19)\n\
#define GENMASK20()		GENMASK(20)\n\
#define GENMASK21()		GENMASK(21)\n\
#define GENMASK22()		GENMASK(22)\n\
#define GENMASK23()		GENMASK(23)\n\
#define GENMASK24()		GENMASK(24)\n\
#define GENMASK25()		GENMASK(25)\n\
#define GENMASK26()		GENMASK(26)\n\
#define GENMASK27()		GENMASK(27)\n\
#define GENMASK28()		GENMASK(28)\n\
#define GENMASK29()		GENMASK(29)\n\
#define GENMASK30()		GENMASK(30)\n\
#define GENMASK31()		GENMASK(31)\n\
#if __WORDSIZE == 32\n\
#  define MININT		0x80000000L\n\
#  define GENMASK32()						\\\n\
    do {							\\\n\
	m = -1;							\\\n\
	t = 0;							\\\n\
    } while (0)\n\
#else\n\
#  define GENMASK32()		GENMASK(32)\n\
#  define GENMASK33()		GENMASK(33)\n\
#  define GENMASK34()		GENMASK(34)\n\
#  define GENMASK35()		GENMASK(35)\n\
#  define GENMASK36()		GENMASK(36)\n\
#  define GENMASK37()		GENMASK(37)\n\
#  define GENMASK38()		GENMASK(38)\n\
#  define GENMASK39()		GENMASK(39)\n\
#  define GENMASK40()		GENMASK(40)\n\
#  define GENMASK41()		GENMASK(41)\n\
#  define GENMASK42()		GENMASK(42)\n\
#  define GENMASK43()		GENMASK(43)\n\
#  define GENMASK44()		GENMASK(44)\n\
#  define GENMASK45()		GENMASK(45)\n\
#  define GENMASK46()		GENMASK(46)\n\
#  define GENMASK47()		GENMASK(47)\n\
#  define GENMASK48()		GENMASK(48)\n\
#  define GENMASK49()		GENMASK(49)\n\
#  define GENMASK50()		GENMASK(50)\n\
#  define GENMASK51()		GENMASK(51)\n\
#  define GENMASK52()		GENMASK(52)\n\
#  define GENMASK53()		GENMASK(53)\n\
#  define GENMASK54()		GENMASK(54)\n\
#  define GENMASK55()		GENMASK(55)\n\
#  define GENMASK56()		GENMASK(56)\n\
#  define GENMASK57()		GENMASK(57)\n\
#  define GENMASK58()		GENMASK(58)\n\
#  define GENMASK59()		GENMASK(59)\n\
#  define GENMASK60()		GENMASK(60)\n\
#  define GENMASK61()		GENMASK(61)\n\
#  define GENMASK62()		GENMASK(62)\n\
#  define GENMASK63()		GENMASK(63)\n\
#  if _WIN32\n\
#    define MININT		0x8000000000000000LL\n\
#  else\n\
#    define MININT		0x8000000000000000L\n\
#  endif\n\
#  define GENMASK64()						\\\n\
    do {							\\\n\
	m = -1;							\\\n\
	t = 0;							\\\n\
    } while (0)\n\
#endif\n\
#if __BYTE_ORDER == __LITTLE_ENDIAN\n\
#  define GENMASK(L)						\\\n\
    do {							\\\n\
	m = (ONE << L) - 1;					\\\n\
	t = m ^ 1;						\\\n\
    } while (0)\n\
#  define SHIFTMASK(B)		m = ~(m << (B))\n\
#else\n\
#  define GENMASK(L)						\\\n\
    do {							\\\n\
	m = (ONE << L) - 1;					\\\n\
	t = m ^ 1;						\\\n\
	m = ((jit_word_t)MININT >> (L - 1));			\\\n\
    } while (0)\n\
#  define SHIFTMASK(B)		m = ~(m >> ((B) - 1))\n\
#endif\n\
\n\
#define	S			jit_word_t\n\
#define	U			jit_uword_t\n\
\n\
#define deftypeSL(L)						\\\n\
typedef union {							\\\n\
    struct {							\\\n\
	S	f: L;						\\\n\
    } b;							\\\n\
    S		s;						\\\n\
} S##L;								\\\n\
static void							\\\n\
CS##L(S##L *u, S (*JS##L)(S##L*)) {				\\\n\
    S t, j, m;							\\\n\
    GENMASK##L();						\\\n\
    m = ~m;							\\\n\
    t = ((t << (__WORDSIZE - L)) >> (__WORDSIZE - L));		\\\n\
    u->s = 0;							\\\n\
    j = (*JS##L)(u);						\\\n\
    if (u->b.f != t || t != j || (u->s & m))			\\\n\
	abort();						\\\n\
}\n\
\n\
#define deftypeSBL(B, L)					\\\n\
typedef union {							\\\n\
    struct {							\\\n\
	S	_: B;						\\\n\
	S	f: L;						\\\n\
    } b;							\\\n\
    S		s;						\\\n\
} S##B##_##L;							\\\n\
static void							\\\n\
CS##B##_##L(S##B##_##L *u, S (*JS##B##_##L)(S##B##_##L*)) {	\\\n\
    S t, j, m;							\\\n\
    GENMASK##L();						\\\n\
    SHIFTMASK(B);						\\\n\
    t = ((t << (__WORDSIZE - L)) >> (__WORDSIZE - L));		\\\n\
    u->s = 0;							\\\n\
    j = (*JS##B##_##L)(u);					\\\n\
    if (u->b.f != t || t != j || (u->s & m))			\\\n\
	abort();						\\\n\
}\n\
\n\
#define deftypeUL(L)						\\\n\
typedef union {							\\\n\
    struct {							\\\n\
	U	f: L;						\\\n\
    } b;							\\\n\
    U		u;						\\\n\
} U##L;								\\\n\
static void							\\\n\
CU##L(U##L *u, U (*JU##L)(U##L*)) {				\\\n\
    U t, j, m;							\\\n\
    GENMASK##L();						\\\n\
    m = ~m;							\\\n\
    t = ((t << (__WORDSIZE - L)) >> (__WORDSIZE - L));		\\\n\
    u->u = 0;							\\\n\
    j = (*JU##L)(u);						\\\n\
    if (u->b.f != t || t != j || (u->u & m))			\\\n\
	abort();						\\\n\
}\n\
\n\
#define deftypeUBL(B, L)					\\\n\
typedef union {							\\\n\
    struct {							\\\n\
	U	_: B;						\\\n\
	U	f: L;						\\\n\
    } b;							\\\n\
    U		u;						\\\n\
} U##B##_##L;							\\\n\
static void							\\\n\
CU##B##_##L(U##B##_##L *u, U (*JU##B##_##L)(U##B##_##L*)) {	\\\n\
    U t, j, m;							\\\n\
    GENMASK##L();						\\\n\
    SHIFTMASK(B);						\\\n\
    t = ((t << (__WORDSIZE - L)) >> (__WORDSIZE - L));		\\\n\
    u->u = 0;							\\\n\
    j = (*JU##B##_##L)(u);					\\\n\
    if (u->b.f != t || t != j || (u->u & m))			\\\n\
	abort();						\\\n\
}");
    puts("\n/* Define signed bitfields at offset 0 */");
    for (i = 1; i <= 32; i++)
	printf("deftypeSL(%d)\n", i);
    /* Avoid clang running out of memory on OpenBSD mips64 */
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (; i <= 64; i++)
	printf("deftypeSL(%d)\n", i);
    puts("#endif");
    puts("/* Define unsigned bitfields at offset 0 */");
    for (i = 1; i <= 32; i++)
	printf("deftypeUL(%d)\n", i);
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (; i <= 64; i++)
	printf("deftypeUL(%d)\n", i);
    puts("#endif");
    for (i = 1; i <= 31; i++) {
	printf("/* Define signed bitfields at offset %d */\n", i);
	for (j = 1; j <= 32 - i; j++)
	    printf("deftypeSBL(%d, %d)\n", i, j);
    }
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (i = 1; i < 64; i++) {
	printf("/* Define signed bitfields at offset %d */\n", i);
	for (j = 1; j <= 64 - i; j++) {
	    if (i + j > 32)
		printf("deftypeSBL(%d, %d)\n", i, j);
	}
    }
    puts("#endif");
    for (i = 1; i <= 31; i++) {
	printf("/* Define unsigned bitfields at offset %d */\n", i);
	for (j = 1; j <= 32 - i; j++)
	    printf("deftypeUBL(%d, %d)\n", i, j);
    }
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (i = 1; i < 64; i++) {
	printf("/* Define unsigned bitfields at offset %d */\n", i);
	for (j = 1; j <= 64 - i; j++) {
	    if (i + j > 32)
		printf("deftypeUBL(%d, %d)\n", i, j);
	}
    }
    puts("#endif");

    puts("\n\
int\n\
main(int argc, char *argv[])\n\
{\n\
    jit_node_t		 *arg;\n\
    jit_node_t		 *jmpi_main;\n\
    jit_state_t		 *_jit;\n\
    void		(*code)(void);");
    puts("    /* Declare signed bitfields at offset 0 */");
    for (i = 1; i <= 32; i++) {
	printf("    S%d			 pS%d;\n", i, i);
	printf("    jit_node_t		*nS%d;\n", i);
    }
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (; i <= 64; i++) {
	printf("    S%d			 pS%d;\n", i, i);
	printf("    jit_node_t		*nS%d;\n", i);
    }
    puts("#endif");
    puts("    /* Declare unsigned bitfields at offset 0 */");
    for (i = 1; i <= 32; i++) {
	printf("    U%d			 pU%d;\n", i, i);
	printf("    jit_node_t		*nU%d;\n", i);
    }
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (; i <= 64; i++) {
	printf("    U%d			 pU%d;\n", i, i);
	printf("    jit_node_t		*nU%d;\n", i);
    }
    puts("#endif");

    for (i = 1; i <= 31; i++) {
	printf("    /* Declare signed bitfields at offset %d */\n", i);
	for (j = 1; j <= 32 - i; j++) {
	    printf("    S%d_%d		 pS%d_%d;\n", i, j, i, j);
	    printf("    jit_node_t\t	*nS%d_%d;\n", i, j);
	}
    }
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (i = 1; i < 64; i++) {
	printf("    /* Declare signed bitfields at offset %d */\n", i);
	for (j = 1; j <= 64 - i; j++) {
	    if (i + j > 32) {
		printf("    S%d_%d		 pS%d_%d;\n", i, j, i, j);
		printf("    jit_node_t\t	*nS%d_%d;\n", i, j);
	    }
	}
    }
    puts("#endif");
    for (i = 1; i <= 31; i++) {
	printf("    /* Declare unsigned bitfields at offset %d */\n", i);
	for (j = 1; j <= 32 - i; j++) {
	    printf("    U%d_%d		 pU%d_%d;\n", i, j, i, j);
	    printf("    jit_node_t\t	*nU%d_%d;\n", i, j);
	}
    }
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (i = 1; i < 64; i++) {
	printf("    /* Declare unsigned bitfields at offset %d */\n", i);
	for (j = 1; j <= 64 - i; j++) {
	    if (i + j > 32) {
		printf("    U%d_%d		 pU%d_%d;\n", i, j, i, j);
		printf("    jit_node_t\t	*nU%d_%d;\n", i, j);
	    }
	}
    }
    puts("#endif\n");

    puts("\
    init_jit(argv[0]);\n\
    _jit = jit_new_state();\n\
\n\
    jmpi_main = jit_jmpi();\n");

    puts("    /* Define jit functions for signed bitfields at offset 0 */");
    for (i = 1; i <= 32; i++) {
	printf("\
    nS%d = jit_label();\n\
    jit_prolog();\n\
    arg = jit_arg();\n\
    jit_getarg(JIT_R0, arg);\n\
    jit_ldr(JIT_R2, JIT_R0);\n", i);
	if ((i >> 3) < sizeof(void*))
	    printf("jit_movi(JIT_R1, ((ONE << %d) - 1) ^ 1);\n", i);
	else
	    printf("jit_movi(JIT_R1, 0);\n");
	printf("\
    jit_depr(JIT_R2, JIT_R1, 0, %d);\n\
    jit_extr(JIT_R1, JIT_R2, 0, %d);\n\
    jit_str(JIT_R0, JIT_R2);\n\
    jit_retr(JIT_R1);\n\
    jit_epilog();\n", i, i);
    }
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (; i <= 64; i++) {
	printf("\
    nS%d = jit_label();\n\
    jit_prolog();\n\
    arg = jit_arg();\n\
    jit_getarg(JIT_R0, arg);\n\
    jit_ldr(JIT_R2, JIT_R0);\n", i);
	if ((i >> 3) < sizeof(void*))
	    printf("jit_movi(JIT_R1, ((ONE << %d) - 1) ^ 1);\n", i);
	else
	    printf("jit_movi(JIT_R1, 0);\n");
	printf("\
    jit_depr(JIT_R2, JIT_R1, 0, %d);\n\
    jit_extr(JIT_R1, JIT_R2, 0, %d);\n\
    jit_str(JIT_R0, JIT_R2);\n\
    jit_retr(JIT_R1);\n\
    jit_epilog();\n", i, i);
    }
    puts("#endif");

    puts("    /* Define jit functions for unsigned bitfields at offset 0 */");
    for (i = 1; i <= 32; i++) {
	printf("\
    nU%d = jit_label();\n\
    jit_prolog();\n\
    arg = jit_arg();\n\
    jit_getarg(JIT_R0, arg);\n\
    jit_ldr(JIT_R2, JIT_R0);\n", i);
	if ((i >> 3) < sizeof(void*))
	    printf("jit_movi(JIT_R1, ((ONE << %d) - 1) ^ 1);\n", i);
	else
	    printf("jit_movi(JIT_R1, 0);\n");
	printf("\
    jit_depr(JIT_R2, JIT_R1, 0, %d);\n\
    jit_extr_u(JIT_R1, JIT_R2, 0, %d);\n\
    jit_str(JIT_R0, JIT_R2);\n\
    jit_retr(JIT_R1);\n\
    jit_epilog();\n", i, i);
    }
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (; i <= 64; i++) {
	printf("\
    nU%d = jit_label();\n\
    jit_prolog();\n\
    arg = jit_arg();\n\
    jit_getarg(JIT_R0, arg);\n\
    jit_ldr(JIT_R2, JIT_R0);\n", i);
	if ((i >> 3) < sizeof(void*))
	    printf("jit_movi(JIT_R1, ((ONE << %d) - 1) ^ 1);\n", i);
	else
	    printf("jit_movi(JIT_R1, 0);\n");
	printf("\
    jit_depr(JIT_R2, JIT_R1, 0, %d);\n\
    jit_extr_u(JIT_R1, JIT_R2, 0, %d);\n\
    jit_str(JIT_R0, JIT_R2);\n\
    jit_retr(JIT_R1);\n\
    jit_epilog();\n", i, i);
    }
    puts("#endif");

    for (i = 1; i <= 31; i++) {
	printf("    /* Define jit functions for signed bitfields at offset %d */\n", i);
	for (j = 1; j <= 32 - i; j++) {
	printf("\
    nS%d_%d = jit_label();\n\
    jit_prolog();\n\
    arg = jit_arg();\n\
    jit_getarg(JIT_R0, arg);\n\
    jit_ldr(JIT_R2, JIT_R0);\n\
    jit_movi(JIT_R1, ((ONE << %d) - 1) ^ 1);\n\
    jit_depr(JIT_R2, JIT_R1, %d, %d);\n\
    jit_extr(JIT_R1, JIT_R2, %d, %d);\n\
    jit_str(JIT_R0, JIT_R2);\n\
    jit_retr(JIT_R1);\n\
    jit_epilog();\n", i, j, j, i, j, i, j);
	}
    }
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (i = 1; i < 64; i++) {
	printf("    /* Declare jit functions for signed bitfields at offset %d */\n", i);
	for (j = 1; j <= 64 - i; j++) {
	    if (i + j > 32)
		printf("\
    nS%d_%d = jit_label();\n\
    jit_prolog();\n\
    arg = jit_arg();\n\
    jit_getarg(JIT_R0, arg);\n\
    jit_ldr(JIT_R2, JIT_R0);\n\
    jit_movi(JIT_R1, ((ONE << %d) - 1) ^ 1);\n\
    jit_depr(JIT_R2, JIT_R1, %d, %d);\n\
    jit_extr(JIT_R1, JIT_R2, %d, %d);\n\
    jit_str(JIT_R0, JIT_R2);\n\
    jit_retr(JIT_R1);\n\
    jit_epilog();\n", i, j, j, i, j, i, j);
	}
    }
    puts("#endif");

    for (i = 1; i <= 31; i++) {
	printf("    /* Define jit functions for unsigned bitfields at offset %d */\n", i);
	for (j = 1; j <= 32 - i; j++) {
	printf("\
    nU%d_%d = jit_label();\n\
    jit_prolog();\n\
    arg = jit_arg();\n\
    jit_getarg(JIT_R0, arg);\n\
    jit_ldr(JIT_R2, JIT_R0);\n\
    jit_movi(JIT_R1, ((ONE << %d) - 1) ^ 1);\n\
    jit_depr(JIT_R2, JIT_R1, %d, %d);\n\
    jit_extr_u(JIT_R1, JIT_R2, %d, %d);\n\
    jit_str(JIT_R0, JIT_R2);\n\
    jit_retr(JIT_R1);\n\
    jit_epilog();\n", i, j, j, i, j, i, j);
	}
    }
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (i = 1; i < 64; i++) {
	printf("    /* Declare jit functions for unsigned bitfields at offset %d */\n", i);
	for (j = 1; j <= 64 - i; j++) {
	    if (i + j > 32)
		printf("\
    nU%d_%d = jit_label();\n\
    jit_prolog();\n\
    arg = jit_arg();\n\
    jit_getarg(JIT_R0, arg);\n\
    jit_ldr(JIT_R2, JIT_R0);\n\
    jit_movi(JIT_R1, ((ONE << %d) - 1) ^ 1);\n\
    jit_depr(JIT_R2, JIT_R1, %d, %d);\n\
    jit_extr_u(JIT_R1, JIT_R2, %d, %d);\n\
    jit_str(JIT_R0, JIT_R2);\n\
    jit_retr(JIT_R1);\n\
    jit_epilog();\n", i, j, j, i, j, i, j);
	}
    }
    puts("#endif");

    puts("\n\
    jit_patch(jmpi_main);\n\
    jit_name(\"main\");\n\
    jit_note(\"cbit.c\", __LINE__);\n\
    jit_prolog();\n");

    puts("    /* Call C functions for signed bitfields at offset 0 */");
    for (i = 1; i <= 32; i++)
	printf("\
    jit_patch_at(jit_movi(JIT_R0, 0), nS%d);\n\
    jit_prepare();\n\
    jit_pushargi((jit_word_t)&pS%d);\n\
    jit_pushargr(JIT_R0);\n\
    jit_finishi((jit_word_t*)CS%d);\n", i, i, i);
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (; i <= 64; i++)
	printf("\
    jit_patch_at(jit_movi(JIT_R0, 0), nS%d);\n\
    jit_prepare();\n\
    jit_pushargi((jit_word_t)&pS%d);\n\
    jit_pushargr(JIT_R0);\n\
    jit_finishi((jit_word_t*)CS%d);\n", i, i, i);
    puts("#endif");
    puts("    /* Call C functions for unsigned bitfields at offset 0 */");
    for (i = 1; i <= 32; i++)
	printf("\
    jit_patch_at(jit_movi(JIT_R0, 0), nU%d);\n\
    jit_prepare();\n\
    jit_pushargi((jit_word_t)&pU%d);\n\
    jit_pushargr(JIT_R0);\n\
    jit_finishi((jit_word_t*)CU%d);\n", i, i, i);
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (; i <= 64; i++)
	printf("\
    jit_patch_at(jit_movi(JIT_R0, 0), nU%d);\n\
    jit_prepare();\n\
    jit_pushargi((jit_word_t)&pU%d);\n\
    jit_pushargr(JIT_R0);\n\
    jit_finishi((jit_word_t*)CU%d);\n", i, i, i);
    puts("#endif");

    for (i = 1; i <= 31; i++) {
	printf("    /* Call C functions for signed bitfields at offset %d */\n", i);
	for (j = 1; j <= 32 - i; j++) {
	    printf("\
    jit_patch_at(jit_movi(JIT_R0, 0), nS%d_%d);\n\
    jit_prepare();\n\
    jit_pushargi((jit_word_t)&pS%d_%d);\n\
    jit_pushargr(JIT_R0);\n\
    jit_finishi((jit_word_t*)CS%d_%d);\n", i, j, i, j, i, j);
	}
    }
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (i = 1; i < 64; i++) {
	printf("    /* Call C functions for signed bitfields at offset %d */\n", i);
	for (j = 1; j <= 64 - i; j++) {
	    if (i + j > 32)
		printf("\
    jit_patch_at(jit_movi(JIT_R0, 0), nS%d_%d);\n\
    jit_prepare();\n\
    jit_pushargi((jit_word_t)&pS%d_%d);\n\
    jit_pushargr(JIT_R0);\n\
    jit_finishi((jit_word_t*)CS%d_%d);\n", i, j, i, j, i, j);
	}
    }
    puts("#endif");

    for (i = 1; i <= 31; i++) {
	printf("    /* Call C functions for unsigned bitfields at offset %d */\n", i);
	for (j = 1; j <= 32 - i; j++) {
	    printf("\
    jit_patch_at(jit_movi(JIT_R0, 0), nU%d_%d);\n\
    jit_prepare();\n\
    jit_pushargi((jit_word_t)&pU%d_%d);\n\
    jit_pushargr(JIT_R0);\n\
    jit_finishi((jit_word_t*)CU%d_%d);\n", i, j, i, j, i, j);
	}
    }
    puts("#if __WORDSIZE == 64 && !SKIP_64_BITS");
    for (i = 1; i < 64; i++) {
	printf("    /* Call C functions for unsigned bitfields at offset %d */\n", i);
	for (j = 1; j <= 64 - i; j++) {
	    if (i + j > 32)
		printf("\
    jit_patch_at(jit_movi(JIT_R0, 0), nU%d_%d);\n\
    jit_prepare();\n\
    jit_pushargi((jit_word_t)&pU%d_%d);\n\
    jit_pushargr(JIT_R0);\n\
    jit_finishi((jit_word_t*)CU%d_%d);\n", i, j, i, j, i, j);
	}
    }
    puts("#endif");

    puts("\n\
    jit_ret();\n\
    jit_epilog();\n\
\n\
    code = jit_emit();\n\
\n\
    jit_clear_state();\n\
\n\
    (*code)();\n\
    jit_destroy_state();\n\
\n\
    finish_jit();\n\
\n\
    return (0);\n\
}");

    return (0);
}
